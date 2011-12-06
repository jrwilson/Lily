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
#include <utility>

using namespace std::rel_ops;

placement_alloc::placement_alloc (const void* begin,
				  const void* limit) :
  begin_ (static_cast<const uint8_t*> (begin)),
  end_ (begin_),
  limit_ (static_cast<const uint8_t*> (limit))
{
  kassert (begin_ <= limit_);
}

void*
placement_alloc::alloc (size_t size)
{
  if (size > 0 && static_cast<ptrdiff_t> (size) <= (limit_ - end_)) {
    void* retval = static_cast<void*> (const_cast<uint8_t*> (end_));
    end_ += size;
    return retval;
  }
  else {
    return 0;
  }
}
