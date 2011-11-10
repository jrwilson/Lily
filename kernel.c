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
#include "multiboot_preparse.h"
#include "placement_allocator.h"
#include "frame_manager.h"
#include "multiboot_parse.h"
#include "vm_manager.h"
#include "syscall_handler.h"
#include "system_automaton.h"
#include "page_fault_handler.h"

extern int text_begin;
extern int text_end;

extern int rodata_begin;
extern int rodata_end;

extern int ctor_begin;

extern int data_begin;
extern int data_end;

/* logical_end denotes the end of the logical address space as setup by loader.s.
   The multiboot data structures use physical addresses.
   loader.s identity maps up to (logical_address - KERNEL_VIRTUAL_BASE) so they can be read without manual translation.
 */
extern "C" void
kmain (void* logical_end,
       multiboot_info_t* multiboot_info,
       uint32_t multiboot_bootloader_magic)
{
  disable_interrupts ();

  /* Print a welcome message. */
  clear_console ();
  kputs ("Lily\n");

  /* Initialize memory segmentation. */
  gdt_initialize ();
  /* Initialize interrupts. */
  idt_initialize ();

  /* The next task is to gain control of the memory. */

  /* First, find the area of memory used for multiboot data structures. */
  kassert (multiboot_bootloader_magic == MULTIBOOT_BOOTLOADER_MAGIC);
  size_t multiboot_begin = (size_t)multiboot_info;
  size_t multiboot_end = multiboot_begin + sizeof (multiboot_info_t);

  /* If you are interested in a multiboot data structure, add code to adjust multiboot_begin and multiboot_end here. */
  kassert (multiboot_preparse_memory_map (multiboot_info, &multiboot_begin, &multiboot_end) == 0);

  /* Adjust the limits to fall on page boundaries and convert them to logical addresses. */
  multiboot_begin = PAGE_ALIGN_DOWN (multiboot_begin + KERNEL_VIRTUAL_BASE);
  multiboot_end = PAGE_ALIGN_UP (multiboot_end + KERNEL_VIRTUAL_BASE);

  /* Find the end of the kernel. */
  void* end_of_kernel = (void*)PAGE_ALIGN_UP ((size_t)&data_end);

  /* The logical beginning and end of the data allocated by the placement allocator. */
  void* placement_begin = (end_of_kernel > (void*)multiboot_end) ? end_of_kernel : (void*)multiboot_end;
  void* placement_end;
  {
    /* Create a placement allocator for the frame manager.
       The frame manager can use logical addresses from the end of the multiboot data structures to the logical end. */
    placement_allocator_t placement_allocator;
    placement_allocator_initialize (&placement_allocator, placement_begin, logical_end);

    /* Initialize the frame manager. */
    frame_manager_initialize (&placement_allocator);
    multiboot_parse_memory_map (multiboot_info, &placement_allocator);

    placement_end = placement_allocator_get_marker (&placement_allocator);
  }

  /* Initialize the virtual memory manager. */
  vm_manager_initialize (placement_begin, placement_end);

  kputs ("text "); kputp (&text_begin); kputs ("-"); kputp ((void*)PAGE_ALIGN_UP ((size_t)&text_end)); kputs ("\n");
  kputs ("rodata "); kputp (&rodata_begin); kputs ("-"); kputp ((void*)PAGE_ALIGN_UP ((size_t)&rodata_end)); kputs ("\n");
  kputs ("data "); kputp (&data_begin); kputs ("-"); kputp ((void*)PAGE_ALIGN_UP ((size_t)&data_end)); kputs ("\n");
  kputs ("plac "); kputp (placement_begin); kputs ("-"); kputp ((void*)PAGE_ALIGN_UP ((size_t)placement_end)); kputs ("\n");

  /* Initialize the system call handler. */
  syscall_handler_initialize ();

  /* Initialize the page fault handler. */
  page_fault_handler_initialize ();

  /* Does not return. */
  system_automaton_initialize (placement_begin, placement_end);

  kassert (0);
}
