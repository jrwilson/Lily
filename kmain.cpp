/*
  File
  ----
  kmain.cpp
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "kernel.hpp"
#include "kassert.hpp"

/* logical_end denotes the end of the logical address space as setup by loader.s.
   The multiboot data structures use physical addresses.
   loader.s identity maps up to (logical_address - KERNEL_VIRTUAL_BASE) so they can be read without manual translation.
 */

extern uint32_t multiboot_magic;
extern multiboot_info_t* multiboot_info;
extern void* logical_end;

static kernel kernel (multiboot_magic, multiboot_info, logical_end);

extern "C" void
kmain (void)
{
  kernel.run ();
  kassert (0);
}
