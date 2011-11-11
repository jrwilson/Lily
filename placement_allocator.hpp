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

#include "types.hpp"

typedef struct {
  logical_address begin;
  logical_address end;
  logical_address marker;
} placement_allocator_t;

void
placement_allocator_initialize (placement_allocator_t* placement_allocator,
				logical_address begin,
				logical_address end);

void*
placement_allocator_alloc (placement_allocator_t* placement_allocator,
			   size_t size) __attribute__((warn_unused_result));

logical_address
placement_allocator_get_marker (placement_allocator_t* placement_allocator);

#endif /* __placement_allocator_h__ */
