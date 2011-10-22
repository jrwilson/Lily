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
#include "memory.h"
#include "interrupt.h"
#include "pit.h"
#include "halt.h"

#define MULTIBOOT_MAGIC 0x2BADB002

extern unsigned int stack;

void
kmain (void* mbd,
       unsigned int magic)
{
  disable_interrupts ();
  initialize_paging ();
  install_gdt ();
  install_idt ();
 
  clear_console ();

  kputs ("Lily\n");

  if (magic != MULTIBOOT_MAGIC) {
    /* Something went wrong. */
    kputs ("Multiboot data structure is not valid.\n");
    kputs ("Halting");
    halt ();
  }
 
  kputs ("Multiboot data structure at physical address: ");
  kputux ((unsigned int)mbd);
  kputs ("\n");

  /* You could either use multiboot.h */
  /* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
  /* or do your offsets yourself. The following is merely an example. */ 
  //char * boot_loader_name =(char*) ((long*)mbd)[16];

  kputs ("Stack start: "); kputux ((unsigned int)&stack); kputs ("\n");
  kputs ("Stack limit: "); kputux ((unsigned int)&stack + 0x1000); kputs ("\n");

  /* Unhandled interrupts. */
  /* asm volatile ("int $0x3"); */
  /* asm volatile ("int $0x4"); */

  initialize_pit (0xFFFF);

  enable_interrupts ();

  /* Wait for interrupts. */
  for (;;) {
    __asm__ __volatile__ ("hlt");
  }
}
