/*
  File
  ----
  automaton.h
  
  Description
  -----------
  Functions for manipulating the automaton data structure.

  Authors:
  Justin R. Wilson
*/

#include "automaton.hpp"
#include "kassert.hpp"
#include "scheduler.hpp"
#include "system_automaton.hpp"
#include "gdt.hpp"
#include "page_fault_handler.hpp"
#include "string.hpp"
#include <utility>

using namespace std::rel_ops;

/* Alignment of stack for switch. */
#define STACK_ALIGN 16

automaton::automaton (privilege_t privilege,
		      physical_address page_directory,
		      logical_address stack_pointer,
		      logical_address memory_ceiling,
		      page_privilege_t page_privilege) :
  scheduler_context_ (scheduler::allocate_context (this)),
  page_directory_ (page_directory),
  stack_pointer_ (stack_pointer),
  memory_ceiling_ (memory_ceiling.align_down (PAGE_SIZE)),
  page_privilege_ (page_privilege)
{
  switch (privilege) {
  case RING0:
    code_segment_ = KERNEL_CODE_SELECTOR | RING0;
    stack_segment_ = KERNEL_DATA_SELECTOR | RING0;
    break;
  case RING1:
  case RING2:
    /* These rings are not supported. */
    kassert (0);
    break;
  case RING3:
    /* Not supported yet. */
    kassert (0);
    break;
  }
}

void
automaton::merge (memory_map_type::iterator pos)
{
  if (pos != memory_map_.end ()) {
    memory_map_type::iterator next (pos);
    ++pos;
    /* Can the areas be merged? */
    if (pos != memory_map_.end () &&
	(*pos)->type == VM_AREA_DATA &&
	(*next)->type == VM_AREA_DATA &&
	(*pos)->end == (*next)->begin &&
	(*pos)->page_privilege == (*next)->page_privilege) {
      /* Merge. */
      (*pos)->end = (*next)->end;
      /* Remove. */
      vm_area* temp = *next;
      memory_map_.erase (next);
      delete temp;
    }
  }
}

struct compare_vm_area {
  bool
  operator () (const vm_area* const x,
	       const vm_area* const y) const
  {
    return x->begin < y->begin;
  }
};

int
automaton::insert_vm_area (const vm_area& area)
{
  kassert (area.end <= memory_ceiling_);

  // Find the location to insert.
  memory_map_type::iterator pos = std::lower_bound (memory_map_.begin (), memory_map_.end (), &area, compare_vm_area ());

  // Check for conflicts.
  if (pos != memory_map_.begin ()) {
    memory_map_type::const_iterator prev (pos);
    --prev;
    if ((*prev)->intersects (area)) {
      return -1;
    }
  }
  if (pos != memory_map_.end ()) {
    if ((*pos)->intersects (area)) {
      return -1;
    }
  }

  // Insert.
  pos = memory_map_.insert (pos, new vm_area (area));

  /* Merge. */
  merge (pos);
  merge (--pos);

  return 0;
}

/* Create or expand a data area for the requested size. */
logical_address
automaton::alloc (size_t size,
		  syserror_t* error)
{
  if (physical_address (size).is_aligned (PAGE_SIZE)) {

    for (memory_map_type::iterator pos = memory_map_.begin ();
	 pos != memory_map_.end ();
	 ++pos) {
      /* Start with the floor and ceiling of the automaton. */
      logical_address ceiling = memory_ceiling_;
      memory_map_type::iterator next (pos);
      ++next;
      if (next != memory_map_.end ()) {
	/* Ceiling drops so we don't interfere with next area. */
	ceiling = (*next)->begin;
      }

      if (size <= (size_t)(ceiling - (*pos)->end)) {
	logical_address retval = (*pos)->end;
	if ((*pos)->type == VM_AREA_DATA) {
	  /* Extend a data area. */
	  (*pos)->end += size;
	  /* Try to merge with the next area. */
	  merge (pos);
	}
	else {
	  /* Insert after the current area. */
	  pos = memory_map_.insert (next, new vm_area (VM_AREA_DATA, retval, retval + size, page_privilege_));
	  /* Try to merge with the next area. */
	  merge (pos);
	}
	*error = SYSERROR_SUCCESS;
	return retval;
      }
    }

    *error = SYSERROR_OUT_OF_MEMORY;
    return logical_address ();
  }
  else {
    *error = SYSERROR_REQUESTED_SIZE_NOT_ALIGNED;
    return logical_address ();
  }
}

/* Reserve a region of memory. */
logical_address
automaton::reserve (size_t size)
{
  kassert (physical_address (size).is_aligned (PAGE_SIZE));

  for (memory_map_type::iterator pos = memory_map_.begin ();
       pos != memory_map_.end ();
       ++pos) {
    memory_map_type::iterator next (pos);
    ++next;
    /* Start with the floor and ceiling of the automaton. */
    logical_address ceiling = memory_ceiling_;
    if (next != memory_map_.end ()) {
      /* Ceiling drops so we don't interfere with next area. */
      ceiling = (*next)->begin;
    }
    
    if (size <= (size_t)(ceiling - (*pos)->end)) {
      logical_address retval = (*pos)->end;
      /* Insert after the current area. */
      memory_map_.insert (next, new vm_area (VM_AREA_RESERVED, retval, retval + size, page_privilege_));
      return retval;
    }
  }
  
  return logical_address ();
}

void
automaton::unreserve (logical_address address)
{
  kassert (address != logical_address ());
  kassert (address.is_aligned (PAGE_SIZE));

  vm_area k (VM_AREA_RESERVED, address, address + PAGE_SIZE, SUPERVISOR);

  memory_map_type::iterator pos = std::lower_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());
  kassert (pos != memory_map_.end ());
  kassert ((*pos)->begin == address);
  kassert ((*pos)->type == VM_AREA_RESERVED);

  vm_area* temp = (*pos);

  memory_map_.erase (pos);
  delete temp;
}

void
automaton::page_fault (logical_address address,
		       uint32_t error)
{
  kassert (address.is_aligned (PAGE_SIZE));

  vm_area k (VM_AREA_RESERVED, address, address + PAGE_SIZE, SUPERVISOR);

  memory_map_type::const_iterator pos = std::lower_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());

  if (pos != memory_map_.end ()) {
    switch ((*pos)->type) {
    case VM_AREA_TEXT:
      /* TODO. */
      kassert (0);
      break;
    case VM_AREA_RODATA:
      /* TODO. */
      kassert (0);
      break;
    case VM_AREA_DATA:
      /* Fault should come from not being present. */
      kassert ((error & PAGE_PROTECTION_ERROR) == 0);
      /* Fault should come from data. */
      kassert ((error & PAGE_INSTRUCTION_ERROR) == 0);
      /* Back the request with a frame. */
      vm_manager_map (address, frame_manager::alloc (), (*pos)->page_privilege, WRITABLE);
      /* Clear the frame. */
      /* TODO:  This is a long operation.  Move it out of the interrupt handler. */
      memset (address.value (), 0x00, PAGE_SIZE);
      return;
      break;
    case VM_AREA_STACK:
      /* TODO. */
      kassert (0);
      break;
    case VM_AREA_RESERVED:
      /* There is a bug in the kernel.  A reserved memory area was not backed. */
      kassert (0);
      break;
    }
  }
  else {
    /* TODO:  Not in memory map. */
    kassert (0);
  }
}

void
automaton::add_action (void* action_entry_point,
		       action_type_t action_type)
{
  kassert (action_map_.insert (std::make_pair (action_entry_point, action_type)).second);
}

action_type_t
automaton::get_action_type (void* action_entry_point)
{
  action_map_type::const_iterator pos = action_map_.find (action_entry_point);
  if (pos != action_map_.end ()) {
    return pos->second;
  }
  else {
    return NO_ACTION;
  }
}

void
automaton::execute (logical_address switch_stack,
		    size_t switch_stack_size,
		    void* action_entry_point,
		    parameter_t parameter,
		    value_t input_value)
{
  logical_address stack_begin;
  logical_address stack_end;
  logical_address new_stack_begin;
  size_t stack_size;
  size_t idx;

  /* Move the stack into an area mapped in all address spaces so that switching page directories doesn't cause a triple fault. */

  /* Determine the beginning of the stack. */
  asm volatile ("mov %%esp, %0\n" : "=m"(stack_begin));
  /* Determine the end of the stack. */
  stack_end = logical_address (&input_value);
  stack_end += sizeof (size_t);
  /* Compute the beginning of the new stack. */
  stack_size = stack_end - stack_begin;
  /* Use a bigger switch stack. */
  kassert (stack_size < switch_stack_size);
  new_stack_begin = (switch_stack + (switch_stack_size - stack_size)).align_down (STACK_ALIGN);
  /* Copy. */
  for (idx = 0; idx < stack_size; ++idx) {
    new_stack_begin[idx] = stack_begin[idx];
  }

  /* Update the base and stack pointers. */
  asm volatile ("add %0, %%esp\n"
		"add %0, %%ebp\n" :: "r"(new_stack_begin - stack_begin) : "%esp", "memory");

  /* Switch page directories. */
  vm_manager_switch_to_directory (page_directory_);

  /* Load the new stack segment.
     Load the new stack pointer.
     Enable interrupts on return.
     Load the new code segment.
     Load the new instruction pointer.
     Load the parameter.
     Load the value for input actions.
  */
  asm volatile ("mov %0, %%eax\n"
		"mov %%ax, %%ss\n"
		"mov %1, %%eax\n"
		"mov %%eax, %%esp\n"
		"pushf\n"
		"pop %%eax\n"
		"or $0x200, %%eax\n"
		"push %%eax\n"
		"pushl %2\n"
		"pushl %3\n"
		"movl %4, %%ecx\n"
		"movl %5, %%edx\n"
		"iret\n" :: "m"(stack_segment_), "m"(stack_pointer_), "m"(code_segment_), "m"(action_entry_point), "m"(parameter), "m"(input_value));
}
