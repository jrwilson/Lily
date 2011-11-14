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

placement_allocator::placement_allocator (const logical_address& begin,
					  const logical_address& end) :
  begin_ (begin),
  end_ (end),
  marker_ (begin)
{
  kassert (begin <= end);
}

void*
placement_allocator::alloc (size_t size)
{
  if (size > 0 && static_cast<ptrdiff_t> (size) <= (end_ - marker_)) {
    void* retval = marker_.value ();
    marker_ += size;
    return retval;
  }
  else {
    return 0;
  }
}
