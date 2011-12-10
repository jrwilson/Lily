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

extern uint32_t multiboot_magic;
extern multiboot_info_t* multiboot_info;

extern int data_end;

extern int* ctors_begin;
extern int* ctors_end;

extern "C" void
kmain (void)
{
  // Print a welcome message.
  clear_console ();
  kputs ("Lily\n");
  
  // Parse the multiboot data structure.
  multiboot_parser multiboot_parser (multiboot_magic, multiboot_info);

  // Initialize the system memory allocator so as to not stomp on the multiboot data structures.
  system_alloc::initialize (align_up (std::max (static_cast<void*> (&data_end), reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ())), PAGE_SIZE), INITIAL_LOGICAL_LIMIT);

  // Call the static constructors.
  // Static objects can use dynamic memory!!
  for (int** ptr = &ctors_begin; ptr != &ctors_end; ++ptr) {
    void (*ctor) () = reinterpret_cast<void (*) ()> (*ptr);
    ctor ();
  }

  // Set up segmentation for x86, or rather, ignore it.
  global_descriptor_table::install ();

  // Set up interrupt dispatching.
  interrupt_descriptor_table::install ();  
  exception_handler::install ();
  irq_handler::install ();
  trap_handler::install ();

  // Initialize the system that allocates frames (physical pages of memory).
  frame_manager::initialize (multiboot_parser.memory_map_begin (), multiboot_parser.memory_map_end ());

  system_automaton::run ();

  kassert (0);
}
