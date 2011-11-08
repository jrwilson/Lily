#ifndef __placement_allocator_h__
#define __placement_allocator_h__

/*
  File
  ----
  placement_allocator.h
  
  Description
  -----------
  A placement allocator.

  Authors:
  Justin R. Wilson
*/

#include "types.h"

typedef struct {
  void* begin;
  void* end;
  void* marker;
} placement_allocator_t;

void
placement_allocator_initialize (placement_allocator_t* placement_allocator,
				void* begin,
				void* end);

void*
placement_allocator_alloc (placement_allocator_t* placement_allocator,
			   uint32_t size);

void*
placement_allocator_get_marker (placement_allocator_t* placement_allocator);

#endif /* __placement_allocator_h__ */
