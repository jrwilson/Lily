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

#include "automaton.h"
#include "malloc.h"
#include "kassert.h"
#include "scheduler.h"
#include "system_automaton.h"
#include "gdt.h"

void
automaton_initialize (automaton_t* ptr,
		      privilege_t privilege,
		      size_t page_directory,
		      void* stack_pointer,
		      void* memory_ceiling,
		      page_privilege_t page_privilege)
{
  kassert (ptr != 0);

  ptr->list_allocator = 0;
  ptr->privilege = privilege;
  ptr->actions = 0;
  ptr->scheduler_context = 0;
  ptr->page_directory = page_directory;
  ptr->stack_pointer = stack_pointer;
  ptr->memory_map_begin = 0;
  ptr->memory_map_end = 0;
  ptr->memory_ceiling = (void*)PAGE_ALIGN_DOWN ((size_t)memory_ceiling);
  ptr->page_privilege = page_privilege;
}

static size_t
action_entry_point_hash_func (const void* x)
{
  return (size_t)x;
}

static int
action_entry_point_compare_func (const void* x,
				 const void* y)
{
  return x - y;
}

automaton_t*
automaton_allocate (list_allocator_t* list_allocator,
		    privilege_t privilege,
		    size_t page_directory,
		    void* stack_pointer,
		    void* memory_ceiling,
		    page_privilege_t page_privilege)
{
  kassert (list_allocator != 0);
  automaton_t* ptr = list_allocator_alloc (list_allocator, sizeof (automaton_t));
  kassert (ptr != 0);
  automaton_initialize (ptr, privilege, page_directory, stack_pointer, memory_ceiling, page_privilege);
  ptr->list_allocator = list_allocator;
  ptr->actions = hash_map_allocate (list_allocator, action_entry_point_hash_func, action_entry_point_compare_func);
  ptr->scheduler_context = scheduler_allocate_context (list_allocator, ptr);

  return ptr;
}

static void
merge (automaton_t* automaton,
       vm_area_t* area)
{
  kassert (automaton != 0);

  if (area != 0) {
    vm_area_t* next = area->next;
    /* Can the areas be merged? */
    if (next != 0 &&
	area->type == VM_AREA_DATA &&
	next->type == VM_AREA_DATA &&
	area->end == next->begin &&
	area->page_privilege == next->page_privilege &&
	area->writable == next->writable) {
      /* Merge. */
      area->end = next->end;
      /* Remove. */
      area->next = next->next;
      if (area->next != 0) {
	area->next->prev = area;
      }
      else {
	automaton->memory_map_end = area;	
      }
      vm_area_free (automaton->list_allocator, next);
    }
  }
}

int
automaton_insert_vm_area (automaton_t* automaton,
			  const vm_area_t* area)
{
  kassert (automaton != 0);
  kassert (area->end <= automaton->memory_ceiling);

  /* Find the location to insert. */
  vm_area_t** nptr;
  vm_area_t** pptr;
  for (nptr = &automaton->memory_map_begin; (*nptr) != 0 && (*nptr)->begin < area->begin; nptr = &(*nptr)->next) ;;

  if (*nptr == 0) {
    pptr = &automaton->memory_map_end;
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
  vm_area_t* ptr = vm_area_allocate (automaton->list_allocator, area->type, area->begin, area->end, area->page_privilege, area->writable);
  ptr->next = *nptr;
  *nptr = ptr;
  ptr->prev = *pptr;
  *pptr = ptr;

  /* Merge. */
  merge (automaton, ptr);
  merge (automaton, ptr->prev);

  return 0;
}

/* Create or expand a data area for the requested size. */
void*
automaton_alloc (automaton_t* automaton,
		 size_t size,
		 syserror_t* error)
{
  kassert (automaton != 0);
  
  if (IS_PAGE_ALIGNED (size)) {

    vm_area_t* ptr;
    for (ptr = automaton->memory_map_begin; ptr != 0; ptr = ptr->next) {
      /* Start with the floor and ceiling of the automaton. */
      void* ceiling = automaton->memory_ceiling;
      if (ptr->next != 0) {
	/* Ceiling drops so we don't interfere with next area. */
	ceiling = ptr->next->begin;
      }

      if (size <= (size_t)(ceiling - ptr->end)) {
	void* retval = ptr->end;
	if (ptr->type == VM_AREA_DATA) {
	  /* Extend a data area. */
	  ptr->end += size;
	  /* Try to merge with the next area. */
	  merge (automaton, ptr);
	}
	else {
	  /* Insert after the current area. */
	  vm_area_t* area = vm_area_allocate (automaton->list_allocator, VM_AREA_DATA, retval, retval + size, automaton->page_privilege, WRITABLE);
	  kassert (area != 0);

	  area->next = ptr->next;
	  ptr->next = area;

	  if (area->next != 0) {
	    area->prev = area->next->prev;
	    area->next->prev = area;
	  }
	  else {
	    area->prev = automaton->memory_map_end;
	    automaton->memory_map_end = area;
	  }
	  /* Try to merge with the next area. */
	  merge (automaton, area);
	}
	*error = SYSERROR_SUCCESS;
	return retval;
      }
    }

    *error = SYSERROR_OUT_OF_MEMORY;
    return 0;
  }
  else {
    *error = SYSERROR_REQUESTED_SIZE_NOT_ALIGNED;
    return 0;
  }
}

/* Reserve a region of memory. */
void*
automaton_reserve (automaton_t* automaton,
		   size_t size)
{
  kassert (automaton != 0);
  kassert (IS_PAGE_ALIGNED (size));

  vm_area_t* ptr;
  for (ptr = automaton->memory_map_begin; ptr != 0; ptr = ptr->next) {
    /* Start with the floor and ceiling of the automaton. */
    void* ceiling = automaton->memory_ceiling;
    if (ptr->next != 0) {
      /* Ceiling drops so we don't interfere with next area. */
      ceiling = ptr->next->begin;
    }
    
    if (size <= (size_t)(ceiling - ptr->end)) {
      void* retval = ptr->end;
      /* Insert after the current area. */
      vm_area_t* area = vm_area_allocate (automaton->list_allocator, VM_AREA_RESERVED, retval, retval + size, automaton->page_privilege, WRITABLE);
      kassert (area != 0);
      
      area->next = ptr->next;
      ptr->next = area;
      
      if (area->next != 0) {
	area->prev = area->next->prev;
	area->next->prev = area;
      }
      else {
	area->prev = automaton->memory_map_end;
	automaton->memory_map_end = area;
      }

      return retval;
    }
  }
  
  return 0;
}

void
automaton_unreserve (automaton_t* automaton,
		     void* logical_addr)
{
  kassert (automaton != 0);
  kassert (logical_addr != 0);
  kassert (IS_PAGE_ALIGNED ((size_t)logical_addr));

  vm_area_t** nptr;
  vm_area_t** pptr;
  for (nptr = &automaton->memory_map_begin; (*nptr) != 0 && (*nptr)->begin != logical_addr; nptr = &(*nptr)->next) ;;

  kassert (*nptr != 0);
  kassert ((*nptr)->type == VM_AREA_RESERVED);

  vm_area_t* temp = *nptr;
  if (temp->next != 0) {
    pptr = &temp->next->prev;
  }
  else {
    pptr = &automaton->memory_map_end;
  }

  *nptr = temp->next;
  *pptr = temp->prev;

  vm_area_free (automaton->list_allocator, temp);
}

void
automaton_set_action_type (automaton_t* ptr,
			   void* action_entry_point,
			   action_type_t action_type)
{
  kassert (ptr != 0);
  kassert (!hash_map_contains (ptr->actions, action_entry_point));
  hash_map_insert (ptr->actions, action_entry_point, (void *)action_type);
}

action_type_t
automaton_get_action_type (automaton_t* ptr,
			   void* action_entry_point)
{
  kassert (ptr != 0);
  if (hash_map_contains (ptr->actions, (const void*)action_entry_point)) {
    action_type_t retval = (action_type_t)hash_map_find (ptr->actions, (const void*)action_entry_point);
    return retval;
  }
  else {
    return NO_ACTION;
  }
}

void
automaton_execute (automaton_t* ptr,
		   void* action_entry_point,
		   parameter_t parameter,
		   value_t input_value)
{
  kassert (ptr != 0);
  
  unsigned int stack_segment;
  unsigned int code_segment;
  
  switch (ptr->privilege) {
  case RING0:
    stack_segment = KERNEL_DATA_SELECTOR | RING0;
    code_segment = KERNEL_CODE_SELECTOR | RING0;
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

  vm_manager_switch_to_directory (ptr->page_directory);
  asm volatile ("mov %0, %%eax\n"
		"mov %%ax, %%ss\n"
		"mov %1, %%eax\n"
		"mov %%eax, %%esp\n"
		"pushf\n"
		"pop %%eax\n"
		"or $0x200, %%eax\n" /* Enable interupts on return. */
		"push %%eax\n"
		"pushl %2\n"
		"pushl %3\n"
		"movl %4, %%ecx\n"
		"movl %5, %%edx\n"
		"iret\n" :: "m"(stack_segment), "m"(ptr->stack_pointer), "m"(code_segment), "m"(action_entry_point), "m"(parameter), "m"(input_value));
}

