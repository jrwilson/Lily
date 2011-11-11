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

boot_automaton::boot_automaton (void* begin,
				void* end)
{
  vm_area_initialize (&data_,
  		      VM_AREA_DATA,
  		      (void*)PAGE_ALIGN_DOWN ((size_t)begin),
  		      (void*)PAGE_ALIGN_UP ((size_t)end),
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

void*
boot_automaton::get_stack_pointer (void) const
{
  kassert (0);
  return 0;
}
  
int
boot_automaton::insert_vm_area (const vm_area_t*)
{
  kassert (0);
  return -1;
}
  
void*
boot_automaton::alloc (size_t size,
		       syserror_t*)
{
  kassert (IS_PAGE_ALIGNED (size));

  void* retval = data_.end;
  data_.end += size;
  return retval;
}
  
void*
boot_automaton::reserve (size_t)
{
  kassert (0);
  return 0;
}
  
void
boot_automaton::unreserve (void*)
{
  kassert (0);
}

void
boot_automaton::page_fault (void* address,
			    uint32_t error)
{
  kassert (address >= data_.begin);
  kassert (address < data_.end);

  /* Fault should come from not being present. */
  kassert ((error & PAGE_PROTECTION_ERROR) == 0);
  /* Fault should come from data. */
  kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
  /* Back the request with a frame. */
  vm_manager_map (address, frame_manager_alloc (), SUPERVISOR, WRITABLE);
  /* Clear the frame. */
  /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
  memset (address, 0x00, PAGE_SIZE);
}
  
void
boot_automaton::set_action_type (void*,
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
boot_automaton::execute (void*,
			 size_t,
			 void*,
			 parameter_t,
			 value_t)
{
  kassert (0);
}
