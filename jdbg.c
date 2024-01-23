#define JDBG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct jdbg_Breakpoint {
  const char* class;
  const char* method;
  const char* line;
} jdbg_Breakpoint;

typedef struct jdbg_BpList {
  size_t size;
  size_t last;
  jdbg_Breakpoint *items;
} jdbg_BpList;

static jdbg_BpList jdbg_bplist_init() {
  jdbg_BpList list = {.size = 0, .last = 0};
  list.items = calloc(10, sizeof(jdbg_Breakpoint));
  return list;
}

static void bplist_push(jdbg_BpList *arr, jdbg_Breakpoint element) {
  if (arr->last == arr->size) {
    arr->items = realloc(arr->items, (arr->size + 10) * sizeof(jdbg_Breakpoint));
    arr->size += 10;
  }
  arr->items[arr->last] = element;
  arr->last += 1;
}

static jdbg_Breakpoint parse_input(const char* input) { 
  // TODO: rewrite stupid ass method
  char *class_name_ptr = malloc(strlen(input));
  char *method_name_ptr = malloc(strlen(input));
  char *line = malloc(strlen(input));

  int pos = 0;
  for (int i = 0; input[i] != ':'; i++) {
    class_name_ptr[i] = input[i];
    pos++;
  }
  pos += 1;
  int a = 0;
  for (int i = pos; input[i] != ':'; i++) {
    method_name_ptr[a] = input[i];
    a++;
    pos++;
  }
  pos += 1;
  a = 0;
  for (int i = pos; input[i] != '\n'; i++) {
    line[a] = input[i];
    a++;
    pos++;
  }
  jdbg_Breakpoint bp = {.class = class_name_ptr, .method = method_name_ptr, .line = line};
  return bp;
}

jdbg_BpList read_breakpoints() {
  jdbg_BpList bp_list = jdbg_bplist_init();
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen("symbols.dbg", "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);
  while ((read = getline(&line, &len, fp)) != -1) {
    parse_input(line);
    bplist_push(&bp_list, parse_input(line));
  }
  fclose(fp);
  if (line)
      free(line);
  return bp_list;
}

int main(void) {
  read_breakpoints();
  return 0;
}