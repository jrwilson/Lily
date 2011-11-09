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

#include "vm_area.h"
#include "kassert.h"
#include "malloc.h"
#include "system_automaton.h"

void
vm_area_initialize (vm_area_t* ptr,
		    vm_area_type_t type,
		    void* begin,
		    void* end,
		    page_privilege_t page_privilege,
		    writable_t writable)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((size_t)begin));
  kassert (IS_PAGE_ALIGNED ((size_t)end));
  kassert (begin < end);

  ptr->type = type;
  ptr->begin = begin;
  ptr->end = end;
  ptr->page_privilege = page_privilege;
  ptr->writable = writable;
  ptr->prev = 0;
  ptr->next = 0;
}

vm_area_t*
vm_area_allocate (list_allocator_t* list_allocator,
		  vm_area_type_t type,
		  void* begin,
		  void* end,
		  page_privilege_t page_privilege,
		  writable_t writable)
{
  kassert (list_allocator != 0);
  vm_area_t* ptr = list_allocator_alloc (list_allocator, sizeof (vm_area_t));
  kassert (ptr != 0);
  vm_area_initialize (ptr, type, begin, end, page_privilege, writable);
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
