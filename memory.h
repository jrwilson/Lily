#ifndef __memory_h__
#define __memory_h__

/*
  File
  ----
  memory.h
  
  Description
  -----------
  Declarations for functions to manage the physical memory.

  Authors:
  http://wiki.osdev.org/Higher_Half_With_GDT
  Justin R. Wilson
*/

#include "multiboot.h"

#define KERNEL_OFFSET 0xC0000000
#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10

void
initialize_paging ();

void
install_gdt ();

void
extend_identity (unsigned int addr);

void
install_page_fault_handler ();

void
initialize_heap (multiboot_info_t* mbd);

void
dump_heap ();

void*
kmalloc (unsigned int size);

/* Page aligned. */
void*
kmalloc_pa (unsigned int size);

void
kfree (void*);

#endif /* __memory_h__ */
