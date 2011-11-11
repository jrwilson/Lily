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

#include "placement_allocator.hpp"
#include "kassert.hpp"

void
placement_allocator_initialize (placement_allocator_t* ptr,
				logical_address begin,
				logical_address end)
{
  kassert (ptr != 0);
  kassert (begin <= end);
  ptr->begin = begin;
  ptr->end = end;
  ptr->marker = ptr->begin;
}

void*
placement_allocator_alloc (placement_allocator_t* ptr,
			   size_t size)
{
  kassert (ptr != 0);
  
  if (size > 0 && static_cast<ptrdiff_t> (size) <= (ptr->end - ptr->marker)) {
    void* retval = ptr->marker.value ();
    ptr->marker += size;
    return retval;
  }
  else {
    return 0;
  }
}

logical_address
placement_allocator_get_marker (placement_allocator_t* ptr)
{
  kassert (ptr != 0);

  return logical_address (ptr->marker);
}
