/*
  File
  ----
  boot_automaton.cpp
  
  Description
  -----------
  Special automaton for booting.

  Authors:
  Justin R. Wilson
*/

#include "boot_automaton.hpp"
#include "kassert.hpp"
#include "string.hpp"
#include "page_fault_handler.hpp"

boot_automaton::boot_automaton (logical_address begin,
				logical_address end)
{
  vm_area_initialize (&data_,
  		      VM_AREA_DATA,
  		      begin.align_down (PAGE_SIZE),
  		      end.align_up (PAGE_SIZE),
  		      SUPERVISOR);
}

const vm_area_t*
boot_automaton::get_data_area (void) const
{
  return &data_;
}

scheduler_context_t*
boot_automaton::get_scheduler_context (void) const
{
  kassert (0);
  return 0;
}

logical_address
boot_automaton::get_stack_pointer (void) const
{
  kassert (0);
  return logical_address ();
}
  
int
boot_automaton::insert_vm_area (const vm_area_t*)
{
  kassert (0);
  return -1;
}
  
logical_address
boot_automaton::alloc (size_t size,
		       syserror_t*)
{
  kassert (physical_address (size).is_aligned (PAGE_SIZE));

  logical_address retval (data_.end);
  data_.end += size;
  return retval;
}
  
logical_address
boot_automaton::reserve (size_t)
{
  kassert (0);
  return logical_address ();
}
  
void
boot_automaton::unreserve (logical_address)
{
  kassert (0);
}

void
boot_automaton::page_fault (logical_address address,
			    uint32_t error)
{
  kassert (address >= data_.begin);
  kassert (address < data_.end);

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
boot_automaton::add_action (void*,
			    action_type_t)
{
  kassert (0);
}
  
action_type_t
boot_automaton::get_action_type (void*)
{
  kassert (0);
  return NO_ACTION;
}
  
void
boot_automaton::execute (logical_address,
			 size_t,
			 void*,
			 parameter_t,
			 value_t)
{
  kassert (0);
}
