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

#include "vm_manager.h"

typedef enum {
  VM_AREA_TEXT,
  VM_AREA_DATA,
  VM_AREA_STACK,
  VM_AREA_BUFFER,
} vm_area_type_t;

typedef struct vm_area vm_area_t;
struct vm_area {
  vm_area_type_t type;
  void* begin;
  void* end;
  page_privilege_t page_privilege;
  writable_t writable;
  vm_area_t* prev;
  vm_area_t* next;
};

void
vm_area_initialize (vm_area_t* ptr,
		    vm_area_type_t type,
		    void* begin,
		    void* end,
		    page_privilege_t page_privilege,
		    writable_t writable);

vm_area_t*
vm_area_allocate (vm_area_type_t type,
		  void* begin,
		  void* end,
		  page_privilege_t page_privilege,
		  writable_t writable) __attribute__((warn_unused_result));

void
vm_area_free (vm_area_t* ptr);

#endif /* __vm_area_h__ */
