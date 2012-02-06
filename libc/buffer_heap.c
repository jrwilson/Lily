#include "buffer_heap.h"

static inline void*
align_up (void* address,
	  unsigned int radix)
{
  return (void*) (((size_t)address + radix - 1) & ~(radix - 1));
}

void
buffer_heap_init (buffer_heap_t* heap,
		  void* begin,
		  size_t size)
{
  /* 4 byte alignment */
  heap->begin = begin;
  heap->end = begin + size;
  heap->mark = align_up (begin, 4);
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
  heap->mark = align_up (heap->mark + size, 4);

  return retval;
}
