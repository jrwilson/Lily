#ifndef __vm_manager_h__
#define __vm_manager_h__

/*
  File
  ----
  vm_manager.h
  
  Description
  -----------
  The virtual memory manager manages logical memory.

  Authors:
  Justin R. Wilson
*/

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
#define KERNEL_VIRTUAL_BASE 0xC0000000

#define PAGE_ALIGN(addr) ((addr) & 0xFFFFF000)

/* Convert physical addresses to frame numbers and vice versa. */
#define ADDRESS_TO_FRAME(addr) ((addr) >> 12)
#define FRAME_TO_ADDRESS(addr) ((addr) << 12)

void
vm_manager_initialize (void);

void*
expand_heap (unsigned int size) __attribute__((warn_unused_result));

#endif /* __vm_manager_h__ */
