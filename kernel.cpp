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

#include "kput.hpp"
#include "kassert.hpp"
#include "multiboot.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "multiboot_preparse.hpp"
#include "placement_allocator.hpp"
#include "frame_manager.hpp"
#include "multiboot_parse.hpp"
#include "vm_manager.hpp"
#include "syscall_handler.hpp"
#include "system_automaton.hpp"
#include "page_fault_handler.hpp"

#include <algorithm>

extern int text_begin;
extern int text_end;

extern int rodata_begin;
extern int rodata_end;

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
  physical_address multiboot_begin_paddr (reinterpret_cast<size_t> (multiboot_info));
  physical_address multiboot_end_paddr (reinterpret_cast<size_t> (multiboot_info) + sizeof (multiboot_info_t));

  /* If you are interested in a multiboot data structure, add code to adjust multiboot_begin and multiboot_end here. */
  kassert (multiboot_preparse_memory_map (multiboot_info, multiboot_begin_paddr, multiboot_end_paddr) == 0);

  /* Adjust the limits to fall on page boundaries and convert them to logical addresses. */
  logical_address multiboot_begin (KERNEL_VIRTUAL_BASE + multiboot_begin_paddr.value ());
  multiboot_begin.align_down (PAGE_SIZE);
  logical_address multiboot_end (KERNEL_VIRTUAL_BASE + multiboot_end_paddr.value ());
  multiboot_end.align_up (PAGE_SIZE);

  /* Find the end of the kernel. */
  logical_address end_of_kernel (&data_end);
  end_of_kernel.align_up (PAGE_SIZE);

  /* The logical beginning and end of the data allocated by the placement allocator. */
  logical_address placement_begin = std::max (end_of_kernel, multiboot_end);
  logical_address placement_end;
  {
    /* Create a placement allocator for the frame manager.
       The frame manager can use logical addresses from the end of the multiboot data structures to the logical end. */
    placement_allocator placement_allocator (placement_begin, logical_address (logical_end));

    /* Initialize the frame manager. */
    multiboot_parse_memory_map (multiboot_info, placement_allocator);

    placement_end = placement_allocator.get_marker ();
  }

  /* Initialize the virtual memory manager. */
  vm_manager_initialize (placement_begin, placement_end);

  kputs ("text "); kputp (&text_begin); kputs ("-"); kputp (&text_end); kputs ("\n");
  kputs ("rodata "); kputp (&rodata_begin); kputs ("-"); kputp (&rodata_end); kputs ("\n");
  kputs ("data "); kputp (&data_begin); kputs ("-"); kputp (&data_end); kputs ("\n");
  kputs ("plac "); kputp (placement_begin.value ()); kputs ("-"); kputp (placement_end.value ()); kputs ("\n");

  /* Initialize the system call handler. */
  syscall_handler_initialize ();

  /* Initialize the page fault handler. */
  page_fault_handler_initialize ();

  /* Does not return. */
  system_automaton_initialize (placement_begin, placement_end);

  kassert (0);
}
