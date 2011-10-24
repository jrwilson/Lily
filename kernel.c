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

  clear_console ();
  kputs ("Lily\n");

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    /* Something went wrong. */
    kputs ("Multiboot data structure is not valid.\n");
    kputs ("Halting");
    halt ();
  }

  identity_map_up_to ((unsigned int)mbd + sizeof (multiboot_info_t));

  kputs ("flags: "); kputux (mbd->flags); kputs ("\n");
  kputs ("mem_lower: "); kputux (mbd->mem_lower); kputs ("\n");
  kputs ("mem_upper: "); kputux (mbd->mem_upper); kputs ("\n");
  kputs ("boot_device: "); kputux (mbd->boot_device); kputs ("\n");
  kputs ("cmdline: "); kputux (mbd->cmdline); kputs ("\n");

  kputs ("Memory Map\n");
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
    kputs ("size: "); kputux (ptr->size); kputs (" addr len type\n");
    ptr = (multiboot_memory_map_t*) ((unsigned int)ptr + ptr->size);
  }

  /* unsigned int k; */
  /* for (k = 0; k < mbd->mmap_length / sizeof (multiboot_memory_map_t); ++k) { */
  /*   kputs ("Size: "); kputux (mmm[k].size); kputs (" addr"); kputs (" len"); kputs (" type"); kputs ("\n"); */
  /* } */

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

  /* You could either use multiboot.h */
  /* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
  /* or do your offsets yourself. The following is merely an example. */ 
  //char * boot_loader_name =(char*) ((long*)mbd)[16];

  kputs ("Stack start: "); kputux ((unsigned int)&stack); kputs ("\n");
  kputs ("Stack limit: "); kputux ((unsigned int)&stack + 0x1000); kputs ("\n");

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
