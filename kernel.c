/*
  File
  ----
  kernel.c
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "kput.h"
#include "kassert.h"
#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "frame_manager.h"
#include "vm_manager.h"
#include "syscall_handler.h"
#include "system_automaton.h"

void
kmain (const multiboot_info_t* mbd,
       unsigned int magic)
{
  kassert (magic == MULTIBOOT_BOOTLOADER_MAGIC);

  clear_console ();
  kputs ("Lily\n");

  gdt_initialize ();
  idt_initialize ();
  frame_manager_initialize (mbd);
  vm_manager_initialize ();
  syscall_handler_initialize ();
  /* Does not return. */
  system_automaton_initialize ();
}
