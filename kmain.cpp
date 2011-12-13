/*
  File
  ----
  kmain.cpp
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "multiboot_parser.hpp"
#include "system_allocator.hpp"
#include "vm_def.hpp"
#include "global_descriptor_table.hpp"
#include "interrupt_descriptor_table.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "frame_manager.hpp"
#include "system_automaton.hpp"
#include "kassert.hpp"

extern int data_end;

extern int* ctors_begin;
extern int* ctors_end;

extern "C" void
kmain (uint32_t multiboot_magic,
       multiboot_info_t* multiboot_info)  // Physical address.
{
  // Print a welcome message.
  clear_console ();
  kputs ("Lily\n");
  
  // Parse the multiboot data structure.
  multiboot_parser multiboot_parser (multiboot_magic, multiboot_info);

  kputs ("Multiboot data (physical) ["); kputx32 (multiboot_parser.begin ()), kputs (" ,"); kputx32 (multiboot_parser.end ()); kputs (")\n");
  kputs ("Multiboot data  (logical) ["); kputx32 (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.begin ()); kputs (" ,"); kputx32 (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ()); kputs (")\n");

  // Initialize the system memory allocator so as to not stomp on the multiboot data structures.
  void* const heap_begin = align_up (std::max (static_cast<void*> (&data_end), reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ())), PAGE_SIZE);
  const void* const heap_end = INITIAL_LOGICAL_LIMIT;
  kputs ("Initial heap ["); kputp (heap_begin); kputs (", "); kputp (heap_end); kputs (")\n");
  system_alloc::initialize (heap_begin, heap_end);

  // Call the static constructors.
  // Static objects can use dynamic memory!!
  kputs ("Constructing static objects\n");
  for (int** ptr = &ctors_begin; ptr != &ctors_end; ++ptr) {
    void (*ctor) () = reinterpret_cast<void (*) ()> (*ptr);
    ctor ();
  }

  // Set up segmentation for x86, or rather, ignore it.
  kputs ("Installing GDT\n");
  global_descriptor_table::install ();

  // Set up interrupt dispatching.
  kputs ("Installing IDT\n");
  interrupt_descriptor_table::install ();  
  kputs ("Installing exception handler\n");
  exception_handler::install ();
  kputs ("Installing irq handler\n");
  irq_handler::install ();
  kputs ("Installing trap handler\n");
  trap_handler::install ();

  // Initialize the system that allocates frames (physical pages of memory).
  kputs ("Parsing memory map\n");
  frame_manager::initialize (multiboot_parser.memory_map_begin (), multiboot_parser.memory_map_end ());

  system_automaton::run ();

  kassert (0);
}
