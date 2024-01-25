#ifndef JDBG_H
#define JDBG_H

#define JDBG_API extern

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct jdbg_breakpoint {
  const char* class;
  const char* method;
  const char* line;
} jdbg_breakpoint;

typedef struct jdbg_bplist {
  size_t size;
  size_t last;
  jdbg_breakpoint *items;
} jdbg_bplist;

typedef struct jdbg_conf {
  jdbg_bplist bp_list;
} jdbg_conf;

typedef enum JDBG_RESULT {
  JDBG_RESULT_OK = 0,
  JDBG_RESULT_ERR = 1,
  JDBG_RESULT_FILE_ERR = 2,
  JDBG_RESULT_MEM_ERR = 3
} JDBG_RESULT;

JDBG_API void insert_breakpoint(jdbg_conf *conf, jdbg_breakpoint breakpoint);
JDBG_API void insert_breakpoint_manual(jdbg_conf *conf, const char *class_name, const char *method_name, int line);
JDBG_API JDBG_RESULT read_breakpoints_from_file(jdbg_conf *conf, char* path);

static void bplist_push(jdbg_bplist *arr, jdbg_breakpoint element);
static jdbg_breakpoint parse_input(const char* input);

/* DECL END */

static jdbg_bplist jdbg_bplist_init() {
  jdbg_bplist list = {.size = 0, .last = 0};
  list.items = calloc(10, sizeof(jdbg_breakpoint));
  return list;
}

static void bplist_push(jdbg_bplist *arr, jdbg_breakpoint element) {
  if (arr->last == arr->size) {
    arr->items = realloc(arr->items, (arr->size + 10) * sizeof(jdbg_breakpoint));
    arr->size += 10;
  }
  arr->items[arr->last] = element;
  arr->last += 1;
}

static jdbg_breakpoint parse_input(const char* input) { 
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
  jdbg_breakpoint bp = {.class = class_name_ptr, .method = method_name_ptr, .line = line};
  return bp;
}

JDBG_RESULT read_breakpoints_from_file(jdbg_conf *conf, char *path) {
  jdbg_bplist bp_list = jdbg_bplist_init();
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(path, "r");
  if (fp == NULL) return JDBG_RESULT_FILE_ERR;
  
  while ((read = getline(&line, &len, fp)) != -1) {
    parse_input(line);
    bplist_push(&bp_list, parse_input(line));
  }
  fclose(fp);
  if (line)
      free(line);
  conf->bp_list = bp_list; 
  return JDBG_RESULT_OK;
}

#endif