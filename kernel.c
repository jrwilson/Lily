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

#define MULTIBOOT_MAGIC 0x2BADB002

void
disable_interrupts ()
{
  __asm__ __volatile__ ("cli");
}

void
halt ()
{
  disable_interrupts ();
  for (;;) {
    __asm__ __volatile__ ("hlt");
  }
}

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

  asm volatile ("int $0x3");
  asm volatile ("int $0x4");

  halt ();
}
