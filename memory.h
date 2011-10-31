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
#define USER_STACK_LIMIT 0x0

#define DESCRIPTOR_COUNT 6
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x18
#define USER_DATA_SELECTOR 0x20
#define TSS_SELECTOR 0x28

typedef struct page_directory page_directory_t;

extern page_directory_t* kernel_page_directory;

void
initialize_paging (void);

void
install_gdt (void);

void
extend_identity (unsigned int addr);

void
install_page_fault_handler (void);

page_directory_t*
allocate_page_directory (void);

void
switch_to_page_directory (page_directory_t* ptr);

void
initialize_heap (multiboot_info_t* mbd);

void
dump_heap (void);

void*
kmalloc (unsigned int size);

/* Page aligned. */
void*
kmalloc_pa (unsigned int size);

void
kfree (void*);

#endif /* __memory_h__ */
