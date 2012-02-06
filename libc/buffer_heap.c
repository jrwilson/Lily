#include "buffer_heap.h"

void
buffer_heap_init (buffer_heap_t* heap,
		  void* begin,
		  size_t size)
{
  /* 4 byte alignment */
  heap->begin = begin;
  heap->end = begin + size;
  heap->mark = begin;
}

void*
buffer_heap_alloc (buffer_heap_t* heap,
		   size_t size)
{
  if (heap->mark >= heap->end) {
    /* The heap is full. */
    return 0;
  }

  if (heap->mark + size > heap->end) {
    /* Not enough space. */
    return 0;
  }

  void* retval = heap->mark;
  heap->mark = heap->mark + size;

  return retval;
}

void*
buffer_heap_begin (const buffer_heap_t* heap)
{
  return heap->begin;
}

bool
buffer_heap_check (const buffer_heap_t* heap,
		   const void* ptr,
		   size_t size)
{
  return ptr >= heap->begin && ptr < heap->end && ptr + size <= heap->end;
}
