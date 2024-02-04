#define JDBG
#include <stdio.h>
#include <jvmti.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <ranges>

struct Break {
  //std::string class_name;
  std::string method_name;
  std::string line;
};

static std::map<std::string, std::vector<Break>> CLASS_BP_MAP;

std::map<std::string, std::vector<Break>> load_breakpoints_from_table(std::string sym_table) {
  auto break_map = std::map<std::string, std::vector<Break>>();
  std::istringstream iss(sym_table);
  std::string line;

  std::string class_name;
  while(std::getline(iss, line, '\n')) {
    std::istringstream lineStream(line);
    std::string token;
    Break br;
    std::getline(lineStream, class_name, ':');

    std::getline(lineStream, br.method_name, ':');
    lineStream >> br.line;
    if(break_map.find(class_name) != break_map.end()) {
      break_map.at(class_name).push_back(br);
    } else {
      break_map.emplace(class_name, std::vector<Break>());
      break_map.at(class_name).push_back(br);
    }
  }

  return break_map;
}

void JNICALL vmInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
  jint class_count;
  jclass* classes;
  if (jvmti_env->GetLoadedClasses(&class_count, &classes) != JVMTI_ERROR_NONE) {
    printf("Failed to get loaded classes\n");
    return;
  }
  jvmti_env->Deallocate((unsigned char*) classes);

  jthread *thread_list;
  jint c;
  jvmti_env->GetAllThreads(&c, &thread_list);
  for(int i = 0; i < c; i++) {
    jvmtiThreadInfo info;
    jvmti_env->GetThreadInfo(thread_list[i], &info);
    printf("thread: %s\n", info.name);
  }
  jvmti_env->Deallocate((unsigned char*)thread_list);

  printf("JDBG AGENT: VMInit event received\n");
  for (int i = 0; i < class_count; i++) {
    char* class_name;
    if (jvmti_env->GetClassSignature(classes[i], &class_name, NULL) == JVMTI_ERROR_NONE) {
      jvmti_env->Deallocate((unsigned char*)class_name);
    }
  }

  jclass source = jni_env->FindClass("Source");
  if (source == NULL) { 
    printf("class not found\n"); 
    return;
  }

  jint error2 = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, thread);
}

void JNICALL Breakpoint(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, jlocation location) {
  printf("breakpoint hit\n");
  jint entry_count;
  jvmtiLocalVariableEntry *variables;
  jvmti_env->GetLocalVariableTable(method, &entry_count, &variables);
  for (int i = 0; i < entry_count; i++) {
    jvmtiLocalVariableEntry j = variables[i];
    printf("[debug] [var] name : %s\n", j.name);
  }
}

void JNICALL ClassPrepare(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jclass klass) {
  jint       method_counter;
  jmethodID  *method_ptr;
  char       *signature_ptr;

  jvmtiError err = jvmti_env->GetClassSignature(klass, &signature_ptr, NULL);
  auto       signature = std::string(signature_ptr);

  if (CLASS_BP_MAP.find(signature) != CLASS_BP_MAP.end()) {
    auto bp_methods = CLASS_BP_MAP.at(signature);
    jvmti_env->GetClassMethods(klass, &method_counter, &method_ptr);

    for(int i = 0; i < method_counter; i++) {
      char* method_name;
      jvmti_env->GetMethodName(method_ptr[i], &method_name, NULL, NULL);

      for(auto &bp : bp_methods) {
        if (std::string(method_name) == bp.method_name) {
          jvmtiLineNumberEntry *line_entry;
          jint entry_count;
          jvmti_env->GetLineNumberTable(method_ptr[i], &entry_count, &line_entry);
          for(size_t line_index = 0; line_index < entry_count; line_index++) {
            if (line_entry[line_index].line_number == std::stoi(bp.line)) {
              std::cout << "Setting breakpoint at: " << bp.method_name << " line: " << bp.line << '\n';
              jint err = jvmti_env->SetBreakpoint(method_ptr[i], line_entry[line_index].start_location);
              if(err != JVMTI_ERROR_NONE) {
                std::cout << "Error while setting breakpoint: " << err << '\n';
              }
            }
          }
        }
      }
    }
 }
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  printf("JDBG AGENT: Loading\n");
  std::ifstream t("symbols.dbg");
  std::string str;

  t.seekg(0, std::ios::end);   
  str.reserve(t.tellg());
  t.seekg(0, std::ios::beg);

  std::string data;
  data.reserve(t.tellg());

  data.assign((std::istreambuf_iterator<char>(t)),
              std::istreambuf_iterator<char>());

  CLASS_BP_MAP = load_breakpoints_from_table(data);
  std::cout << "Loaded breakpoint list successfuly\n";

  jvmtiEnv *env = nullptr;
  vm->GetEnv((void**)&env, JVMTI_VERSION_11); 

  jvmtiCapabilities capa;
  capa.can_access_local_variables = 1;
  capa.can_signal_thread = 1;
  capa.can_generate_breakpoint_events = 1;
  capa.can_tag_objects = 1;
  capa.can_get_monitor_info = 1;
  capa.can_generate_garbage_collection_events = 1;
  capa.can_retransform_any_class = 1;
  capa.can_tag_objects = 1;
  capa.can_generate_all_class_hook_events = 1;
  capa.can_get_line_numbers = 1;
  capa.can_get_thread_cpu_time = 1;
  capa.can_get_current_thread_cpu_time = 1;
  capa.can_suspend = 1;

  env->AddCapabilities(&capa);

  jint error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
  error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread)NULL);
  error = env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, (jthread)NULL);
  if (error != JVMTI_ERROR_NONE) {
    printf("JDBG Agent error: %i", error);
    return JNI_ABORT;
  }

  jvmtiEventCallbacks callbacks;
  callbacks.VMInit = &vmInit;
  callbacks.Breakpoint = &Breakpoint;
  callbacks.ClassPrepare = &ClassPrepare;
     
  env->SetEventCallbacks(&callbacks, sizeof(callbacks));
  return JNI_OK;
}

JNIEXPORT void JNICALL 
Agent_OnUnload(JavaVM *vm) {
  printf("JDBG AGENT: Unloaded\n");
}
