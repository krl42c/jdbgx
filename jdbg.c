#define JDBG
#include <stdio.h>
#include <jvmti.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for usleep

static jrawMonitorID lock;
typedef struct jvm_data {
  jvmtiEnv *env;
  JavaVM *vm;
} jvm_data;

jvm_data gdata;

void JNICALL vmInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
  jint class_count;
  jclass* classes;
  if ((*jvmti_env)->GetLoadedClasses(jvmti_env, &class_count, &classes) != JVMTI_ERROR_NONE) {
    printf("Failed to get loaded classes\n");
    return;
  }
  (*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)classes);

  jthread *thread_list;
  jint c;
  (*jvmti_env)->GetAllThreads(jvmti_env, &c, &thread_list);
  for(int i = 0; i < c; i++) {
    jvmtiThreadInfo info;
    (*jvmti_env)->GetThreadInfo(jvmti_env, thread_list[i], &info);
    printf("thread: %s\n", info.name);
  }
  (*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)thread_list);

  printf("JDBG AGENT: VMInit event received\n");
  for (int i = 0; i < class_count; i++) {
    char* class_name;
    if ((*jvmti_env)->GetClassSignature(jvmti_env, classes[i], &class_name, NULL) == JVMTI_ERROR_NONE) {
      (*jvmti_env)->Deallocate(jvmti_env, (unsigned char*)class_name);
    }
  }

  jclass source = (*jni_env)->FindClass(jni_env, "Source");
  if (source == NULL) { printf("class not found\n"); }

  jint error2 = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, thread);
}

void JNICALL Breakpoint(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, jlocation location) {
  printf("breakpoint hit\n");
  jint entry_count;
  jvmtiLocalVariableEntry *variables;
  (*jvmti_env)->GetLocalVariableTable(jvmti_env, method, &entry_count, &variables);
  for (int i = 0; i < entry_count; i++) {
    jvmtiLocalVariableEntry j = variables[i];
    printf("name : %s\n", j.name);
  }
}

void JNICALL ClassPrepare(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jclass klass) {
  jint method_counter;
  jmethodID *method_ptr;
  if ((*jvmti_env)->GetClassMethods(jvmti_env, klass, &method_counter, &method_ptr) != JVMTI_ERROR_NONE) {
    printf("Failed to get class methods\n");
    return;
  }
  for (int i = 0; i < method_counter; i++) {
    char *name;
    (*jvmti_env)->GetMethodName(jvmti_env, method_ptr[i], &name, NULL, NULL);
    if (strcmp(name, "example") == 0) {
      jvmtiLineNumberEntry *line_entry;
      jint entry_count;
      (*jvmti_env)->GetLineNumberTable(jvmti_env, method_ptr[i], &entry_count, &line_entry);
      jlocation first = line_entry[2] .start_location;
      (*jvmti_env)->SetBreakpoint(jvmti_env, method_ptr[i], first);
      break;
    }
  }
}

void JNICALL SingleStep(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, jlocation location) {
  
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  jvm_data global_data;
  printf("JDBG AGENT: Loading\n");
  (*vm)->GetEnv(vm, &global_data.env, JVMTI_VERSION_1_0); 
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

  (*global_data.env)->AddCapabilities(global_data.env, &capa);

  jint error = (*global_data.env)->SetEventNotificationMode(global_data.env, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
  error = (*global_data.env)->SetEventNotificationMode(global_data.env, JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread)NULL);
  error = (*global_data.env)->SetEventNotificationMode(global_data.env, JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, (jthread)NULL);
  error = (*global_data.env)->SetEventNotificationMode(global_data.env, JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, (jthread)NULL);
  if (error != JVMTI_ERROR_NONE) {
    printf("JDBG Agent error: %i", error);
    return JNI_ABORT;
  }

  jvmtiEventCallbacks callbacks;
  callbacks.VMInit = &vmInit;
  callbacks.Breakpoint = &Breakpoint;
  callbacks.SingleStep = &SingleStep;
  callbacks.ClassPrepare = &ClassPrepare;

  (*global_data.env)->SetEventCallbacks(global_data.env, &callbacks, sizeof(callbacks));
  gdata.vm = vm;
  return JNI_OK;
}

JNIEXPORT void JNICALL 
Agent_OnUnload(JavaVM *vm) {
}