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

placement_alloc::placement_alloc (const logical_address& begin,
				  const logical_address& limit) :
  begin_ (begin),
  end_ (begin),
  limit_ (limit)
{
  kassert (begin <= limit);
}

void*
placement_alloc::alloc (size_t size)
{
  if (size > 0 && static_cast<ptrdiff_t> (size) <= (limit_ - end_)) {
    void* retval = end_.value ();
    end_ += size;
    return retval;
  }
  else {
    return 0;
  }
}
