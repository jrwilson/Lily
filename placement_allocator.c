/*
  File
  ----
  placement_allocator.c
  
  Description
  -----------
  A placement allocator.

  Authors:
  Justin R. Wilson
*/

#include "placement_allocator.h"
#include "kassert.h"

void
placement_allocator_initialize (placement_allocator_t* ptr,
				void* begin,
				void* end)
{
  kassert (ptr != 0);
  kassert (begin <= end);
  ptr->begin = begin;
  ptr->end = end;
  ptr->marker = begin;
}

void*
placement_allocator_alloc (placement_allocator_t* ptr,
			   uint32_t size)
{
  kassert (ptr != 0);
  
  if (size > 0 && size <= (uint32_t)(ptr->end - ptr->marker)) {
    void* retval = ptr->marker;
    ptr->marker += size;
    return retval;
  }
  else {
    return 0;
  }
}

void*
placement_allocator_get_marker (placement_allocator_t* ptr)
{
  kassert (ptr != 0);

  return ptr->marker;
}
