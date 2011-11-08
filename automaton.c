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

void
automaton_initialize (automaton_t* ptr,
		      privilege_t privilege,
		      size_t page_directory,
		      void* stack_pointer,
		      void* memory_ceiling)
{
  kassert (ptr != 0);

  ptr->privilege = privilege;
  ptr->actions = 0;
  ptr->scheduler_context = scheduler_allocate_context (ptr);
  ptr->page_directory = page_directory;
  ptr->stack_pointer = stack_pointer;
  ptr->memory_map_begin = 0;
  ptr->memory_map_end = 0;
  ptr->memory_ceiling = memory_ceiling;
}

automaton_t*
automaton_allocate (privilege_t privilege,
		    size_t page_directory,
		    void* stack_pointer,
		    void* memory_ceiling)
{
  automaton_t* ptr = malloc (sizeof (automaton_t));
  kassert (ptr != 0);
  automaton_initialize (ptr, privilege, page_directory, stack_pointer, memory_ceiling);
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
      vm_area_free (next);
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
  vm_area_t* ptr = vm_area_allocate (area->type, area->begin, area->end, area->page_privilege, area->writable);
  ptr->next = *nptr;
  *nptr = ptr;

  ptr->prev = *pptr;
  *pptr = ptr;

  merge (automaton, ptr);
  merge (automaton, ptr->prev);

  return 0;
}

void*
automaton_alloc (automaton_t* automaton,
		 size_t size,
		 syserror_t* error)
{
  kassert (automaton != 0);
  
  if (IS_PAGE_ALIGNED (size)) {
    /* Okay.  Let's look for a data area that we can extend. */
    void* end;
    vm_area_t** ptr;
    for (ptr = &automaton->memory_map_begin; (*ptr) != 0; ptr = &(*ptr)->next) {
      end = (*ptr)->end;
      if ((*ptr)->type == VM_AREA_DATA) {
  	/* Start with the ceiling of the automaton. */
  	void* ceiling = automaton->memory_ceiling;
  	if ((*ptr)->next != 0) {
  	  /* Ceiling drops so we don't interfere with next area. */
  	  ceiling = (*ptr)->next->begin;
  	}
	
  	if (size <= (size_t)(ceiling - (*ptr)->end)) {
  	  /* We can extend. */
  	  void* retval = (*ptr)->end;
  	  (*ptr)->end += size;
  	  return retval;
  	}
      }
    }
    
    if (*ptr == 0) {
      /* TODO:  Automaton doesn't end with a data area.  Try to create a new area. */
      kassert (0);
    }
    
    kassert (0);
  }
  else {
    *error = SYSERROR_REQUESTED_SIZE_NOT_ALIGNED;
    return 0;
  }
}
