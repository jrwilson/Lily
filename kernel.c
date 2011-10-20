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

#define MULTIBOOT_MAGIC 0x2BADB002

void
disable_interrupts ()
{
  __asm__("cli");
}

void
halt ()
{
  disable_interrupts ();
  for (;;) {
    __asm__("hlt");
  }
}

void
kmain (void* mbd,
       unsigned int magic)
{
  initialize_paging ();
  gdt_install ();
 
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

  /* Print out some numbers in hex. */
  int i = 0;
  for (i = 0; i < 16; ++i) {
    kputux (i);
    kputs ("\n");
  }

  halt ();
}
