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

#include "kout.hpp"
#include "multiboot_parser.hpp"
#include "system_allocator.hpp"
#include "vm_def.hpp"
#include "gdt.hpp"
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
  // Parse the multiboot data structure.
  multiboot_parser multiboot_parser (multiboot_magic, multiboot_info);

  // Initialize the system memory allocator so as to not stomp on the multiboot data structures.
  void* const heap_begin = align_up (std::max (static_cast<void*> (&data_end), reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ())), PAGE_SIZE);
  const void* const heap_end = INITIAL_LOGICAL_LIMIT;
  system_alloc::initialize (heap_begin, heap_end);

  // Call the static constructors.
  // Static objects can use dynamic memory!!
  for (int** ptr = &ctors_begin; ptr != &ctors_end; ++ptr) {
    void (*ctor) () = reinterpret_cast<void (*) ()> (*ptr);
    ctor ();
  }

  // Print a welcome message.
  kout << "Lily" << endl;

  kout << "Multiboot data (physical) [" << hexformat (multiboot_parser.begin ()) << ", " << hexformat (multiboot_parser.end ()) << ")" << endl;
  kout << "Multiboot data  (logical) [" << hexformat (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.begin ()) << ", " << hexformat (reinterpret_cast<physical_address_t> (KERNEL_VIRTUAL_BASE) + multiboot_parser.end ()) << ")" << endl;

  kout << "Initial heap [" << hexformat (heap_begin) << ", " << hexformat (heap_end) << ")" << endl;

  // Set up segmentation for x86, or rather, ignore it.
  kout << "Installing GDT" << endl;
  gdt::install ();

  // Set up interrupt dispatching.
  kout << "Installing IDT" << endl;
  interrupt_descriptor_table::install ();  
  kout << "Installing exception handler" << endl;
  exception_handler::install ();
  kout << "Installing irq handler" << endl;
  irq_handler::install ();
  kout << "Installing trap handler" << endl;
  trap_handler::install ();

  // Initialize the system that allocates frames (physical pages of memory).
  kout << "Parsing memory map" << endl;
  frame_manager::initialize (multiboot_parser.memory_map_begin (), multiboot_parser.memory_map_end ());

  system_automaton::run ();

  kassert (0);
}
