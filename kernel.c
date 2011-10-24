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
#include "multiboot.h"

extern unsigned int stack;

void
kmain (multiboot_info_t* mbd,
       unsigned int magic)
{
  disable_interrupts ();
  initialize_paging ();
  install_gdt ();
  install_idt ();
  install_page_fault_handler ();

  clear_console ();
  kputs ("Lily\n");

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    /* Something went wrong. */
    kputs ("Multiboot data structure is not valid.\n");
    kputs ("Halting");
    halt ();
  }

  identity_map_up_to ((unsigned int)mbd + sizeof (multiboot_info_t));

  if (mbd->flags & MULTIBOOT_INFO_MEMORY) {
    kputs ("MULTIBOOT_INFO_MEMORY\n");
    kputs ("mem_lower: "); kputuix (mbd->mem_lower); kputs ("\n");
    kputs ("mem_upper: "); kputuix (mbd->mem_upper); kputs ("\n");
  }

  if (mbd->flags & MULTIBOOT_INFO_BOOTDEV) {
    kputs ("MULTIBOOT_INFO_BOOTDEV\n");
    kputs ("boot_device: "); kputuix (mbd->boot_device); kputs ("\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_CMDLINE) {
    kputs ("MULTIBOOT_INFO_CMDLINE\n");
    char* name = (char*)mbd->cmdline;
    while (1) {
      identity_map_up_to ((unsigned int)name);
      if (*name == 0) {
	break;
      }
      else {
	++name;
      }
    }
    kputs ("cmdline: "); kputs ((char *)mbd->cmdline); kputs ("\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_MODS) {
    kputs ("MULTIBOOT_INFO_MODS\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) {
    kputs ("MULTIBOOT_INFO_INFO_AOUT_SYMS\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_ELF_SHDR) {
    kputs ("MULTIBOOT_INFO_ELF_SHDR\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_MEM_MAP) {
    kputs ("MULTIBOOT_INFO_MEM_MAP\n");

    identity_map_up_to (mbd->mmap_addr + mbd->mmap_length);

    multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)mbd->mmap_addr;
    multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(mbd->mmap_addr + mbd->mmap_length);
    
    /* struct multiboot_mmap_entry */
    /* { */
    /*   multiboot_uint32_t size; */
    /*   multiboot_uint64_t addr; */
    /*   multiboot_uint64_t len; */
    /* #define MULTIBOOT_MEMORY_AVAILABLE              1 */
    /* #define MULTIBOOT_MEMORY_RESERVED               2 */
    /*   multiboot_uint32_t type; */
    /* } __attribute__((packed)); */
    
    while (ptr < limit) {
      kputuix (ptr->addr); kputs ("-"); kputuix (ptr->addr + ptr->len);
      switch (ptr->type) {
      case MULTIBOOT_MEMORY_AVAILABLE:
	kputs (" AVAILABLE");
	break;
      case MULTIBOOT_MEMORY_RESERVED:
	kputs (" RESERVED");
	break;
      default:
	kputs (" UNKNOWN");
	break;
      }
      kputs ("\n");
      /* Move to the next descriptor using the declared size (which for some reason does not include the size of the size member). */
      ptr = (multiboot_memory_map_t*) ((char *)ptr + sizeof (unsigned int) + ptr->size);
    }
  }
  if (mbd->flags & MULTIBOOT_INFO_DRIVE_INFO) {
    kputs ("MULTIBOOT_INFO_DRIVE_INFO\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
    kputs ("MULTIBOOT_INFO_CONFIG_TABLE\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
    kputs ("MULTIBOOT_INFO_BOOT_LOADER_NAME\n");
    char* name = (char*)mbd->boot_loader_name;
    while (1) {
      identity_map_up_to ((unsigned int)name);
      if (*name == 0) {
	break;
      }
      else {
	++name;
      }
    }
    kputs ("boot_loader_name: "); kputs ((char *)mbd->boot_loader_name); kputs ("\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_APM_TABLE) {
    kputs ("MULTIBOOT_INFO_APM_TABLE\n");
  }
  if (mbd->flags & MULTIBOOT_INFO_VIDEO_INFO) {
    kputs ("MULTIBOOT_INFO_VIDEO_INFO\n");
  }

  kputs ("Stack start: "); kputuix ((unsigned int)&stack); kputs ("\n");
  kputs ("Stack limit: "); kputuix ((unsigned int)&stack + 0x1000); kputs ("\n");

  /* Unhandled interrupts. */
  /* asm volatile ("int $0x3"); */
  /* asm volatile ("int $0x4"); */

  /* initialize_pit (0xFFFF); */
  /* enable_interrupts (); */

  /* Wait for interrupts. */
  for (;;) {
    __asm__ __volatile__ ("hlt");
  }
}
