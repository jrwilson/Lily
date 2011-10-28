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
#include "kassert.h"
#include "multiboot.h"
#include "hash_map.h"

unsigned int
hash_func (void* x)
{
  return (unsigned int)x;
}

int
compare_func (void* x,
	      void* y)
{
  return (unsigned int)x - (unsigned int)y;
}

void
kmain (multiboot_info_t* mbd,
       unsigned int magic)
{
  clear_console ();
  kputs ("Lily\n");

  disable_interrupts ();
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

  hash_map_t* hm = allocate_hash_map (hash_func, compare_func);
  kassert (hash_map_size (hm) == 0);
  hash_map_insert (hm, (void *)1, (void *)1);
  hash_map_insert (hm, (void *)2, (void *)2);
  hash_map_insert (hm, (void *)3, (void *)3);
  hash_map_insert (hm, (void *)4, (void *)4);
  hash_map_insert (hm, (void *)5, (void *)5);
  hash_map_insert (hm, (void *)6, (void *)6);
  hash_map_insert (hm, (void *)7, (void *)7);
  hash_map_insert (hm, (void *)8, (void *)8);
  hash_map_insert (hm, (void *)9, (void *)9);
  hash_map_insert (hm, (void *)10, (void *)10);
  hash_map_erase (hm, (void *)5);
  kassert (hash_map_size (hm) == 9);
  kassert (hash_map_find (hm, (void*)1) == (void*)1);
  kassert (hash_map_find (hm, (void*)5) == (void*)0);
  kassert (hash_map_find (hm, (void*)11) == (void*)0);

  /* Unhandled interrupts. */
  /* asm volatile ("int $0x3"); */
  /* asm volatile ("int $0x4"); */

  /* initialize_pit (0xFFFF); */

  /* Wait for interrupts. */
  enable_interrupts ();
  for (;;) {
    __asm__ __volatile__ ("hlt");
  }
}
