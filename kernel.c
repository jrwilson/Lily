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

/* #include "mm.h" */
/* #include "ksyscall.h" */
/* #include "automata.h" */
/* #include "scheduler.h" */

/* extern int producer_init_entry; */
/* extern int producer_produce_entry; */
/* extern int consumer_init_entry; */
/* extern int consumer_consume_entry; */

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

  unsigned int k;
  for (k = PAGE_SIZE; 1; k *= 2) {
    unsigned int* a = expand_heap (k);
    *a = 0x12345678;
    kputuix (*a); kputs ("\n");
  }

  kassert (0);

  /* initialize_syscalls (); */

  /* initialize_automata (); */

  /* aid_t producer = create (RING0); */
  /* set_action_type (producer, (unsigned int)&producer_init_entry, INTERNAL); */
  /* set_action_type (producer, (unsigned int)&producer_produce_entry, OUTPUT); */

  /* aid_t consumer = create (RING0); */
  /* set_action_type (consumer, (unsigned int)&consumer_init_entry, INTERNAL); */
  /* set_action_type (consumer, (unsigned int)&consumer_consume_entry, INPUT); */

  /* bind (producer, (unsigned int)&producer_produce_entry, 0, consumer, (unsigned int)&consumer_consume_entry, 0); */

  /* initialize_scheduler (); */

  /* schedule_action (producer, (unsigned int)&producer_init_entry, 0); */
  /* schedule_action (consumer, (unsigned int)&consumer_init_entry, 0); */

  /* /\* Start the scheduler.  Doesn't return. *\/ */
  /* finish_action (0, 0); */
}
