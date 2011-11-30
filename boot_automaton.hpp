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

template <class AllocatorTag>
class boot_automaton : public automaton_interface {
private:
  struct data_area : public vm_area_base {
    
    data_area (logical_address begin,
	       logical_address end) :
      vm_area_base (VM_AREA_DATA, begin, end, SUPERVISOR)
    { }

    logical_address
    alloc (size_t size)
    {
      logical_address retval = end_;
      end_ += size;
      end_.align_up (PAGE_SIZE);
      return retval;
    }

    vm_data_area<AllocatorTag>*
    clone () const
    {
      return new vm_data_area<AllocatorTag> (begin_, end_, SUPERVISOR);
    }

    bool
    merge (const vm_area_base&)
    {
      kassert (0);
      return false;
    }

    void
    page_fault (logical_address,
		uint32_t)
    {
      kassert (0);
    }
  };

  data_area data_;
public:
  boot_automaton (logical_address begin,
		  logical_address end) :
    data_ (begin, end)
  { }
  
  const vm_area_base&
  get_data_area (void) const
  {
    return data_;
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
  
  bool
  insert_vm_area (const vm_area_base&)
  {
    kassert (0);
    return false;
  }
  
  logical_address
  alloc (size_t size)
  {
    return data_.alloc (size);
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
  page_fault (logical_address address,
	      uint32_t error)
  {
    kassert (address >= data_.begin ());
    kassert (address < data_.end ());
    
    /* Fault should come from not being present. */
    kassert ((error & PAGE_PROTECTION_ERROR) == 0);
    /* Fault should come from data. */
    kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
    /* Back the request with a frame. */
    vm_manager_map (address, frame_manager::alloc (), SUPERVISOR, WRITABLE);
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
  execute (logical_address,
	   size_t,
	   void*,
	   parameter_t,
	   value_t)
  {
    kassert (0);
  }
  
};

#endif /* __boot_automaton_hpp__ */
