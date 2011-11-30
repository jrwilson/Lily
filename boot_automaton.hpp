#ifndef __boot_automaton_hpp__
#define __boot_automaton_hpp__

/*
  File
  ----
  boot_automaton.hpp
  
  Description
  -----------
  Special automaton for booting.

  Authors:
  Justin R. Wilson
*/

#include "automaton_interface.hpp"
#include "vm_area.hpp"

class boot_automaton : public automaton_interface {
private:

  logical_address begin_;
  logical_address end_;

public:
  boot_automaton (logical_address begin,
		  logical_address end) :
    begin_ (begin >> PAGE_SIZE),
    end_ (end << PAGE_SIZE)
  { }

  inline logical_address
  begin () const
  {
    return begin_;
  }

  inline logical_address
  end () const
  {
    return end_;
  }
  
  void*
  get_scheduler_context (void) const
  {
    kassert (0);
    return 0;
  }
  
  logical_address
  get_stack_pointer (void) const
  {
    kassert (0);
    return logical_address ();
  }
  
  logical_address
  alloc (size_t size)
  {
    logical_address retval = end_;
    end_ += size;
    end_ <<= PAGE_SIZE;
    return retval;
  }
  
  logical_address
  reserve (size_t)
  {
    kassert (0);
    return logical_address ();
  }
  
  void
  unreserve (logical_address)
  {
    kassert (0);
  }
  
  void
  page_fault (frame_manager& fm,
	      vm_manager& vmm,
	      logical_address address,
	      uint32_t error)
  {
    kassert (address >= begin_);
    kassert (address < end_);
    
    /* Fault should come from not being present. */
    kassert ((error & PAGE_PROTECTION_ERROR) == 0);
    /* Fault should come from data. */
    kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
    /* Back the request with a frame. */
    vmm.map (address, fm.alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
    /* Clear the frame. */
    /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
    memset (address.value (), 0x00, PAGE_SIZE);
  }
  
  void
  add_action (void*,
	      action_type_t)
  {
    kassert (0);
  }
  
  action_type_t
  get_action_type (void*)
  {
    kassert (0);
    return NO_ACTION;
  }
  
  void
  execute (vm_manager&,
	   logical_address,
	   size_t,
	   void*,
	   parameter_t,
	   value_t)
  {
    kassert (0);
  }
  
};

#endif /* __boot_automaton_hpp__ */
