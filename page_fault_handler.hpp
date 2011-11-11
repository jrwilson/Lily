#ifndef __page_fault_handler_h__
#define __page_fault_handler_h__

/*
  File
  ----
  page_fault_handler.h
  
  Description
  -----------
  Page fault handler.

  Authors:
  Justin R. Wilson
*/

#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

void
page_fault_handler_initialize (void);

#endif /* __page_fault_handler_h__ */
