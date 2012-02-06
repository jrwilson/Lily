#ifndef BUFFER_HEAP_H
#define BUFFER_HEAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
  void* begin;
  void* end;
  void* mark;
} buffer_heap_t;

void
buffer_heap_init (buffer_heap_t* heap,
		  void* begin,
		  size_t size);

void*
buffer_heap_alloc (buffer_heap_t* heap,
		   size_t size);

void*
buffer_heap_begin (const buffer_heap_t* heap);

bool
buffer_heap_check (const buffer_heap_t* heap,
		   const void* ptr,
		   size_t size);

#endif /* BUFFER_HEAP_H */
