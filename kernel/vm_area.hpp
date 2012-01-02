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
#include "string.hpp"

class vm_area_base {
protected:
  logical_address_t begin_;
  logical_address_t end_;

public:  
  vm_area_base (logical_address_t begin,
		logical_address_t end) :
    begin_ (align_down (begin, PAGE_SIZE)),
    end_ (align_up (end, PAGE_SIZE))
  {
    kassert (begin_ <= end_);
  }
  
  logical_address_t begin () const
  {
    return begin_;
  }
  
  logical_address_t end () const
  {
    return end_;
  }
  
  virtual void
  page_fault (logical_address_t,
	      vm::page_fault_error_t,
	      volatile registers*)
  {
    kassert (0);
  }
};

class vm_text_area : public vm_area_base {
public:
  vm_text_area (logical_address_t begin,
		logical_address_t end) :
    vm_area_base (begin, end)
  { }
};

class vm_rodata_area : public vm_area_base {
public:
  vm_rodata_area (logical_address_t begin,
		  logical_address_t end) :
    vm_area_base (begin, end)
  { }
};

class vm_data_area : public vm_area_base {
public:
  vm_data_area (logical_address_t begin,
		logical_address_t end) :
    vm_area_base (begin, end)
  { }
};

class vm_heap_area : public vm_area_base {
public:
  vm_heap_area (logical_address_t begin) :
    vm_area_base (align_down (begin, PAGE_SIZE), align_down (begin, PAGE_SIZE))
  { }

  void
  set_end (logical_address_t e)
  {
    end_ = e;
    kassert (begin_ <= end_);
  }

  void
  page_fault (logical_address_t address,
	      vm::page_fault_error_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert (vm::not_present (error));
    // Fault should come from data.
    kassert (vm::data_context (error));
    // Back the request with a frame.
    vm::map (address, frame_manager::alloc (), vm::USER, vm::WRITABLE);
    // Clear the frame.
    ltl::memset (reinterpret_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

class vm_stack_area : public vm_area_base {
public:
  vm_stack_area (logical_address_t begin,
		 logical_address_t end) :
    vm_area_base (begin, end)
  { }

  void
  page_fault (logical_address_t address,
	      vm::page_fault_error_t error,
	      volatile registers*)
  {
    // Fault should come from not being present.
    kassert (vm::not_present (error));
    // Fault should come from data.
    kassert (vm::data_context (error));
    // Back the request with a frame.
    vm::map (address, frame_manager::alloc (), vm::USER, vm::WRITABLE);
    // Clear the frame.
    ltl::memset (reinterpret_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
  }
};

#endif /* __vm_area_hpp__ */
