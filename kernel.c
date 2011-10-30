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

#include "descriptor.h"

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

extern int stack_top;
extern int bang_entry;

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

  /* /\* Create a new page directory for a task. *\/ */
  /* page_directory_t* pd = allocate_page_directory (); */

  /* unsigned int amount = 1; */
  /* char flipflop = 0; */
  
  /* while (1) { */
  /*   kmalloc (amount);  */
  /*   /\* dump_heap (); *\/ */
  /*   amount *= 2; */
  /*   if (flipflop) { */
  /*     switch_to_page_directory (pd); */
  /*   } */
  /*   else { */
  /*     switch_to_page_directory (kernel_page_directory); */
  /*   } */
  /*   flipflop = ~flipflop; */
  /* } */

  kputs ("Stack Segment: "); kputuix (KERNEL_DATA_SELECTOR | RING0); kputs ("\n");
  kputs ("Stack Pointer: "); kputp (&stack_top); kputs ("\n");
  kputs ("Flags: (default)\n");
  kputs ("Code Segment: "); kputuix (KERNEL_CODE_SELECTOR | RING0); kputs ("\n");
  kputs ("Instruction Pointer: "); kputp (&bang_entry); kputs ("\n");


  unsigned int stack_segment = KERNEL_DATA_SELECTOR | RING0;
  unsigned int stack_pointer = (unsigned int)&stack_top;
  unsigned int code_segment = KERNEL_CODE_SELECTOR | RING0;
  unsigned int instruction_pointer = (unsigned int)&bang_entry;

  __asm__ __volatile__ ("pushl %0\n"
			"pushl %1\n"
			"pushf\n"
			"push %2\n"
			"push %3\n"
			"iret\n" :: "m"(stack_segment), "m"(stack_pointer), "m"(code_segment), "m"(instruction_pointer));

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
