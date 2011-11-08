#ifndef __list_allocator_h__
#define __list_allocator_h__

/*
  File
  ----
  list_allocator.h
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "types.h"

typedef struct list_allocator list_allocator_t;

list_allocator_t*
list_allocator_allocate (void) __attribute__((warn_unused_result));

void*
list_allocator_alloc (list_allocator_t*,
		      size_t) __attribute__((warn_unused_result));

void
list_allocator_free (list_allocator_t*,
		     void*);

#endif /* __list_allocator_h__ */
