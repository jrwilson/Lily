#ifndef __vm_area_h__
#define __vm_area_h__

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

#include "vm_manager.hpp"
#include "list_allocator.hpp"

typedef enum {
  VM_AREA_TEXT,
  VM_AREA_RODATA,
  VM_AREA_DATA,
  VM_AREA_STACK,
  VM_AREA_RESERVED,
} vm_area_type_t;

typedef struct vm_area vm_area_t;
struct vm_area {
  vm_area_type_t type;
  logical_address begin;
  logical_address end;
  page_privilege_t page_privilege;
  vm_area_t* prev;
  vm_area_t* next;
};

void
vm_area_initialize (vm_area_t* ptr,
		    vm_area_type_t type,
		    logical_address begin,
		    logical_address end,
		    page_privilege_t page_privilege);

vm_area_t*
vm_area_allocate (list_allocator_t* list_allocator,
		  vm_area_type_t type,
		  logical_address begin,
		  logical_address end,
		  page_privilege_t page_privilege) __attribute__((warn_unused_result));

void
vm_area_free (list_allocator_t* list_allocator,
	      vm_area_t* ptr);

#endif /* __vm_area_h__ */