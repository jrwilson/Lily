/*
  File
  ----
  vm_area.h
  
  Description
  -----------
  Describe a region of virtual memory.
  I stole this idea from Linux.

  Authors:
  Justin R. Wilson
*/

#include "vm_area.hpp"
#include "kassert.hpp"
#include "system_automaton.hpp"

void
vm_area_initialize (vm_area_t* ptr,
		    vm_area_type_t type,
		    logical_address begin,
		    logical_address end,
		    page_privilege_t page_privilege)
{
  kassert (ptr != 0);
  kassert (begin.is_aligned (PAGE_SIZE));
  kassert (end.is_aligned (PAGE_SIZE));
  kassert (begin < end);

  ptr->type = type;
  ptr->begin = begin;
  ptr->end = end;
  ptr->page_privilege = page_privilege;
  ptr->prev = 0;
  ptr->next = 0;
}

vm_area_t*
vm_area_allocate (list_allocator_t* list_allocator,
		  vm_area_type_t type,
		  logical_address begin,
		  logical_address end,
		  page_privilege_t page_privilege)
{
  kassert (list_allocator != 0);
  vm_area_t* ptr = static_cast<vm_area_t*> (list_allocator_alloc (list_allocator, sizeof (vm_area_t)));
  kassert (ptr != 0);
  vm_area_initialize (ptr, type, begin, end, page_privilege);
  return ptr;
}

void
vm_area_free (list_allocator_t* list_allocator,
	      vm_area_t* ptr)
{
  kassert (list_allocator != 0);
  kassert (ptr != 0);
  list_allocator_free (list_allocator, ptr);
}
