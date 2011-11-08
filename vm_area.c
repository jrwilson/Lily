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
vm_area_allocate (vm_area_type_t type,
		  void* begin,
		  void* end,
		  page_privilege_t page_privilege,
		  writable_t writable)
{
  vm_area_t* ptr = malloc (sizeof (vm_area_t));
  kassert (ptr != 0);
  vm_area_initialize (ptr, type, begin, end, page_privilege, writable);
  return ptr;
}

void
vm_area_free (vm_area_t* ptr)
{
  kassert (ptr != 0);
  free (ptr);
}
