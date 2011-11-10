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
  uint8_t* begin;
  uint8_t* end;
  uint8_t* marker;
} placement_allocator_t;

void
placement_allocator_initialize (placement_allocator_t* placement_allocator,
				void* begin,
				void* end);

void*
placement_allocator_alloc (placement_allocator_t* placement_allocator,
			   size_t size) __attribute__((warn_unused_result));

void*
placement_allocator_get_marker (placement_allocator_t* placement_allocator);

#endif /* __placement_allocator_h__ */
