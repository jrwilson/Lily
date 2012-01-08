#ifndef __vm_area_hpp__
#define __vm_area_hpp__

/*
  File
  ----
  vm_area.hpp
  
  Description
  -----------
  Describe a region of virtual memory.
  I stole this idea from Linux.

  Authors:
  Justin R. Wilson
*/

#include "vm.hpp"
#include "frame_manager.hpp"
#include "kassert.hpp"
#include <string.h>

class vm_area_base {
protected:
  logical_address_t begin_;
  logical_address_t end_;

public:  
  vm_area_base (logical_address_t begin,
		logical_address_t end) :
    begin_ (begin),
    end_ (end)
  {
    kassert (begin_ <= end_);
  }
  
  logical_address_t
  begin () const
  {
    return begin_;
  }
  
  logical_address_t
  end () const
  {
    return end_;
  }

  void
  set_end (logical_address_t e)
  {
    end_ = e;
    kassert (begin_ <= end_);
  }
};

#endif /* __vm_area_hpp__ */
