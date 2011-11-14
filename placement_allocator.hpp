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

class placement_allocator {
private:
  logical_address begin_;
  logical_address end_;
  logical_address marker_;

public:
  
  placement_allocator (const logical_address& begin,
		       const logical_address& end);
  
  void*
  alloc (size_t size) __attribute__((warn_unused_result));
  
  inline logical_address
  get_marker (void) const {
    return marker_;
  }
};


#endif /* __placement_allocator_h__ */
