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

void
initialize_paging ();

void
gdt_install ();

#endif /* __memory_h__ */
