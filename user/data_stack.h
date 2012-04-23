#ifndef DATA_STACK_H
#define DATA_STACK_H

#include <buffer_file.h>

typedef enum {
  TABLE,
  STRING,
  INTEGER,
} object_type_t;

typedef struct pair pair_t;

typedef struct {
  object_type_t type;
  union {
    struct {
      pair_t* head;
    } table;
    struct {
      char* str;
      size_t size;
    } string;
    struct {
      int i;
    } integer;
  } u;
} object_t;

struct pair {
  object_t* key;
  object_t* value;
  pair_t* next;
};

typedef struct {
  buffer_file_t bf;
  object_t** stack;
  size_t stack_size;
  size_t stack_capacity;
} data_stack_t;

int
data_stack_initw (data_stack_t* ds,
		  bd_t bd);

int
data_stack_push_table (data_stack_t* ds);

int
data_stack_push_string (data_stack_t* ds,
			const char* str);

int
data_stack_push_integer (data_stack_t* ds,
			 int i);

int
data_stack_insert (data_stack_t* ds);

int
data_stack_initr (data_stack_t* ds,
		  bd_t bd);

#endif /* DATA_STACK_H */
