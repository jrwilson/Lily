#ifndef __mm_h__
#define __mm_h__

/*
  File
  ----
  memory.h
  
  Description
  -----------
  Declarations for the memory manager.

  Authors:
  http://wiki.osdev.org/Higher_Half_With_GDT
  Justin R. Wilson
*/

#include "multiboot.h"

#define USER_STACK_LIMIT 0x0

typedef struct page_directory page_directory_t;

void
initialize_identity_map (void);

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

#endif /* __mm_h__ */
