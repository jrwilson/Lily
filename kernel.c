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
#include "kassert.h"
#include "multiboot.h"
#include "ksyscall.h"
#include "automata.h"
#include "scheduler.h"

#include "hash_map.h"

extern int producer_init_entry;
extern int producer_produce_entry;
extern int consumer_init_entry;
extern int consumer_consume_entry;

void
kmain (multiboot_info_t* mbd,
       unsigned int magic)
{
  clear_console ();
  kputs ("Lily\n");

  initialize_paging ();
  install_gdt ();
  install_idt ();
  install_page_fault_handler ();

  kassert (magic == MULTIBOOT_BOOTLOADER_MAGIC);

  /* Extend memory to cover the GRUB data structures. */
  extend_identity ((unsigned int)mbd + sizeof (multiboot_info_t) - 1);

  if (mbd->flags & MULTIBOOT_INFO_MEMORY) {
    kputs ("mem_lower: "); kputuix (mbd->mem_lower); kputs ("\n");
    kputs ("mem_upper: "); kputuix (mbd->mem_upper); kputs ("\n");
  }

  if (mbd->flags & MULTIBOOT_INFO_BOOTDEV) {
    kputs ("boot_device: "); kputuix (mbd->boot_device); kputs ("\n");
  }

  if (mbd->flags & MULTIBOOT_INFO_CMDLINE) {
    char* cmdline = (char*)mbd->cmdline;
    extend_identity ((unsigned int)cmdline);
    for (; *cmdline != 0; ++cmdline) {
      extend_identity ((unsigned int)cmdline);
    }
    kputs ("cmdline: "); kputs ((char *)mbd->cmdline); kputs ("\n");
  }

  /* if (mbd->flags & MULTIBOOT_INFO_MODS) { */
  /*   kputs ("MULTIBOOT_INFO_MODS\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) { */
  /*   kputs ("MULTIBOOT_INFO_INFO_AOUT_SYMS\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_ELF_SHDR) { */
  /*   kputs ("MULTIBOOT_INFO_ELF_SHDR\n"); */
  /* } */

  if (mbd->flags & MULTIBOOT_INFO_MEM_MAP) {
    extend_identity (mbd->mmap_addr + mbd->mmap_length - 1);
  }

  /* if (mbd->flags & MULTIBOOT_INFO_DRIVE_INFO) { */
  /*   kputs ("MULTIBOOT_INFO_DRIVE_INFO\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_CONFIG_TABLE) { */
  /*   kputs ("MULTIBOOT_INFO_CONFIG_TABLE\n"); */
  /* } */

  if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
    char* name = (char*)mbd->boot_loader_name;
    extend_identity ((unsigned int)name);
    for (; *name != 0; ++name) {
      extend_identity ((unsigned int)name);
    }
    kputs ("boot_loader_name: "); kputs ((char *)mbd->boot_loader_name); kputs ("\n");
  }

  /* if (mbd->flags & MULTIBOOT_INFO_APM_TABLE) { */
  /*   kputs ("MULTIBOOT_INFO_APM_TABLE\n"); */
  /* } */
  /* if (mbd->flags & MULTIBOOT_INFO_VIDEO_INFO) { */
  /*   kputs ("MULTIBOOT_INFO_VIDEO_INFO\n"); */
  /* } */

  initialize_heap (mbd);

  initialize_syscalls ();

  initialize_automata ();

  aid_t producer = create (RING0);
  set_action_type (producer, (unsigned int)&producer_init_entry, INTERNAL);
  set_action_type (producer, (unsigned int)&producer_produce_entry, OUTPUT);

  aid_t consumer = create (RING0);
  set_action_type (consumer, (unsigned int)&consumer_init_entry, INTERNAL);
  set_action_type (consumer, (unsigned int)&consumer_consume_entry, INPUT);

  bind (producer, (unsigned int)&producer_produce_entry, 0, consumer, (unsigned int)&consumer_consume_entry, 0);

  initialize_scheduler ();

  schedule_action (producer, (unsigned int)&producer_init_entry, 0);
  schedule_action (consumer, (unsigned int)&consumer_init_entry, 0);

  /* Start the scheduler.  Doesn't return. */
  finish_action (0, 0);
}
