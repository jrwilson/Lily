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

/* Alignment of stack for switch. */
#define STACK_ALIGN 16

static size_t
action_entry_point_hash_func (const void* x)
{
  return (size_t)x;
}

static int
action_entry_point_compare_func (const void* x,
				 const void* y)
{
  return static_cast<const uint8_t*> (x) - static_cast<const uint8_t*> (y);
}

automaton::automaton (list_allocator& list_allocator,
		      privilege_t privilege,
		      physical_address page_directory,
		      logical_address stack_pointer,
		      logical_address memory_ceiling,
		      page_privilege_t page_privilege) :
  list_allocator_ (list_allocator),
  actions_ (hash_map_allocate (&list_allocator, action_entry_point_hash_func, action_entry_point_compare_func)),
  scheduler_context_ (scheduler_allocate_context (list_allocator, this)),
  page_directory_ (page_directory),
  stack_pointer_ (stack_pointer),
  memory_map_begin_ (0),
  memory_map_end_ (0),
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
automaton::merge (vm_area_t* area)
{
  if (area != 0) {
    vm_area_t* next = area->next;
    /* Can the areas be merged? */
    if (next != 0 &&
	area->type == VM_AREA_DATA &&
	next->type == VM_AREA_DATA &&
	area->end == next->begin &&
	area->page_privilege == next->page_privilege) {
      /* Merge. */
      area->end = next->end;
      /* Remove. */
      area->next = next->next;
      if (area->next != 0) {
	area->next->prev = area;
      }
      else {
	memory_map_end_ = area;	
      }
      vm_area_free (list_allocator_, next);
    }
  }
}

int
automaton::insert_vm_area (const vm_area_t* area)
{
  kassert (area->end <= memory_ceiling_);

  /* Find the location to insert. */
  vm_area_t** nptr;
  vm_area_t** pptr;
  for (nptr = &memory_map_begin_; (*nptr) != 0 && (*nptr)->begin < area->begin; nptr = &(*nptr)->next) ;;

  if (*nptr == 0) {
    pptr = &memory_map_end_;
  }
  else {
    pptr = &(*nptr)->prev;
  }

  /* Check for conflicts. */
  if ((*nptr != 0 && (*nptr)->begin < area->end) ||
      (*pptr != 0 && area->begin < (*pptr)->end)) {
    return -1;
  }

  /* Insert. */
  vm_area_t* ptr = vm_area_allocate (list_allocator_, area->type, area->begin, area->end, area->page_privilege);
  ptr->next = *nptr;
  *nptr = ptr;
  ptr->prev = *pptr;
  *pptr = ptr;

  /* Merge. */
  merge (ptr);
  merge (ptr->prev);

  return 0;
}

/* Create or expand a data area for the requested size. */
logical_address
automaton::alloc (size_t size,
		  syserror_t* error)
{
  if (physical_address (size).is_aligned (PAGE_SIZE)) {

    vm_area_t* ptr;
    for (ptr = memory_map_begin_; ptr != 0; ptr = ptr->next) {
      /* Start with the floor and ceiling of the automaton. */
      logical_address ceiling = memory_ceiling_;
      if (ptr->next != 0) {
	/* Ceiling drops so we don't interfere with next area. */
	ceiling = ptr->next->begin;
      }

      if (size <= (size_t)(ceiling - ptr->end)) {
	logical_address retval = ptr->end;
	if (ptr->type == VM_AREA_DATA) {
	  /* Extend a data area. */
	  ptr->end += size;
	  /* Try to merge with the next area. */
	  merge (ptr);
	}
	else {
	  /* Insert after the current area. */
	  vm_area_t* area = vm_area_allocate (list_allocator_, VM_AREA_DATA, retval, retval + size, page_privilege_);
	  kassert (area != 0);

	  area->next = ptr->next;
	  ptr->next = area;

	  if (area->next != 0) {
	    area->prev = area->next->prev;
	    area->next->prev = area;
	  }
	  else {
	    area->prev = memory_map_end_;
	    memory_map_end_ = area;
	  }
	  /* Try to merge with the next area. */
	  merge (area);
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

  vm_area_t* ptr;
  for (ptr = memory_map_begin_; ptr != 0; ptr = ptr->next) {
    /* Start with the floor and ceiling of the automaton. */
    logical_address ceiling = memory_ceiling_;
    if (ptr->next != 0) {
      /* Ceiling drops so we don't interfere with next area. */
      ceiling = ptr->next->begin;
    }
    
    if (size <= (size_t)(ceiling - ptr->end)) {
      logical_address retval = ptr->end;
      /* Insert after the current area. */
      vm_area_t* area = vm_area_allocate (list_allocator_, VM_AREA_RESERVED, retval, retval + size, page_privilege_);
      kassert (area != 0);
      
      area->next = ptr->next;
      ptr->next = area;
      
      if (area->next != 0) {
	area->prev = area->next->prev;
	area->next->prev = area;
      }
      else {
	area->prev = memory_map_end_;
	memory_map_end_ = area;
      }

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

  vm_area_t** nptr;
  vm_area_t** pptr;
  for (nptr = &memory_map_begin_; (*nptr) != 0 && (*nptr)->begin != address; nptr = &(*nptr)->next) ;;

  kassert (*nptr != 0);
  kassert ((*nptr)->type == VM_AREA_RESERVED);

  vm_area_t* temp = *nptr;
  if (temp->next != 0) {
    pptr = &temp->next->prev;
  }
  else {
    pptr = &memory_map_end_;
  }

  *nptr = temp->next;
  *pptr = temp->prev;

  vm_area_free (list_allocator_, temp);
}

void
automaton::page_fault (logical_address address,
		       uint32_t error)
{
  vm_area_t* ptr;
  for (ptr = memory_map_begin_; ptr != 0 && (address < ptr->begin || address >= ptr->end); ptr = ptr->next) ;;

  if (ptr != 0) {
    switch (ptr->type) {
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
      vm_manager_map (address, frame_manager::alloc (), ptr->page_privilege, WRITABLE);
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
  kassert (!hash_map_contains (actions_, action_entry_point));
  hash_map_insert (actions_, action_entry_point, (void *)action_type);
}

action_type_t
automaton::get_action_type (void* action_entry_point)
{
  if (hash_map_contains (actions_, (const void*)action_entry_point)) {
    void* x = hash_map_find (actions_, (const void*)action_entry_point);
    action_type_t* p = reinterpret_cast<action_type_t*> (&x);
    action_type_t retval = *p;
    return retval;
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
