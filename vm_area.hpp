#ifndef __vm_area_hpp__
#define __vm_area_hpp__

/*
  File
  ----
  vm_area.hpp
  
  Description
  -----------
  Describe a region of virtual memory.
  I stole this idea from Linux.

  Authors:
  Justin R. Wilson
*/

#include "vm_manager.hpp"
#include "kassert.hpp"

typedef enum {
  VM_AREA_TEXT,
  VM_AREA_RODATA,
  VM_AREA_DATA,
  VM_AREA_STACK,
  VM_AREA_RESERVED,
} vm_area_type_t;

struct vm_area {
  vm_area_type_t type;
  logical_address begin;
  logical_address end;
  page_privilege_t page_privilege;
  vm_area* prev;
  vm_area* next;

  vm_area (vm_area_type_t t,
	   logical_address b,
	   logical_address e,
	   page_privilege_t pp) :
    type (t),
    begin (b),
    end (e),
    page_privilege (pp),
    prev (0),
    next (0)
  {
    kassert (begin.is_aligned (PAGE_SIZE));
    kassert (end.is_aligned (PAGE_SIZE));
    kassert (begin < end);
  }

};

#endif /* __vm_area_hpp__ */
