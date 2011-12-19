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
#include "buffer.hpp"

class vm_area_base {
protected:
  const void* begin_;
  const void* end_;

public:  
  vm_area_base (const void* begin,
		const void* end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }
  
  const void* begin () const
  {
    return begin_;
  }
  
  const void* end () const
  {
    return end_;
  }
  
  virtual void
  page_fault (const void*,
	      page_fault_error_t,
	      volatile registers*)
  {
    kassert (0);
  }
};

class vm_text_area : public vm_area_base {
public:
  vm_text_area (const void* begin,
		const void* end) :
    vm_area_base (begin, end)
  { }
};

class vm_rodata_area : public vm_area_base {
public:
  vm_rodata_area (const void* begin,
		  const void* end) :
    vm_area_base (begin, end)
  { }
};

class vm_data_area : public vm_area_base {
public:
  vm_data_area (const void* begin,
		const void* end) :
    vm_area_base (begin, end)
  { }

  const void* begin () const
  {
    return begin_;
  }

  const void* end () const
  {
    return end_;
  }
};

class vm_heap_area : public vm_area_base {
private:
  const vm::page_privilege_t page_privilege_;

public:
  vm_heap_area (const void* begin,
		vm::page_privilege_t page_privilege) :
    vm_area_base (align_down (begin, PAGE_SIZE), align_down (begin, PAGE_SIZE)),
    page_privilege_ (page_privilege)
  { }

  void
  set_end (const void* e)
  {
    end_ = e;
    kassert (begin_ <= end_);
  }

  void
  page_fault (const void* address,
	      page_fault_error_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert (not_present (error));
    // Fault should come from data.
    kassert (data_context (error));
    // Back the request with a frame.
    vm::map (address, frame_manager::alloc (), page_privilege_, vm::WRITABLE);
    // Clear the frame.
    memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

class vm_stack_area : public vm_area_base {
private:
  const vm::page_privilege_t page_privilege_;

public:
  vm_stack_area (const void* begin,
		 const void* end,
		 vm::page_privilege_t page_privilege) :
    vm_area_base (begin, end),
    page_privilege_ (page_privilege)
  { }

  void
  page_fault (const void* address,
	      page_fault_error_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert (not_present (error));
    // Fault should come from data.
    kassert (data_context (error));
    // Back the request with a frame.
    vm::map (address, frame_manager::alloc (), page_privilege_, vm::WRITABLE);
    // Clear the frame.
    memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

class vm_buffer_area : public vm_area_base {
private:
  buffer* buffer_;

public:
  vm_buffer_area (const void* begin,
		  buffer* b) :
    vm_area_base (begin, static_cast<const uint8_t*> (begin) + b->size ()),
    buffer_ (b)
  { }

  void
  page_fault (const void* address,
	      page_fault_error_t error,
	      volatile registers*)
  {
    buffer_->page_fault (begin_, address, error);
  }

};

#endif /* __vm_area_hpp__ */
