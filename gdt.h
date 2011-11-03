#ifndef __gdt_h__
#define __gdt_h__

/*
  File
  ----
  gdt.h
  
  Description
  -----------
  Functions for the Global Descriptor Table (GDT).

  Authors:
  Justin R. Wilson
*/

/* Should be consistent with segments.s. */
#define DESCRIPTOR_COUNT 5
/* Null selector is 0x00. */
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x18
#define USER_DATA_SELECTOR 0x20

void
initialize_gdt (void);

#endif /* __gdt_h__ */
