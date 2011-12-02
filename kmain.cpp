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

#include "global_descriptor_table.hpp"
#include "interrupt_descriptor_table.hpp"
#include "multiboot_parser.hpp"
#include "placement_allocator.hpp"
#include "vm_def.hpp"
#include "frame_manager.hpp"
#include "vm_manager.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "system_automaton.hpp"
#include "kassert.hpp"

/* logical_end denotes the end of the logical address space as setup by loader.s.
   The multiboot data structures use physical addresses.
   loader.s identity maps up to (logical_address - KERNEL_VIRTUAL_BASE) so they can be read without manual translation.
 */

extern uint32_t multiboot_magic;
extern multiboot_info_t* multiboot_info;

extern int data_end;

extern "C" void
kmain (void)
{
  // Print a welcome message.
  clear_console ();
  kputs ("Lily\n");
  
  global_descriptor_table::install ();
  interrupt_descriptor_table::install ();
  
  multiboot_parser multiboot_parser_ (multiboot_magic, multiboot_info);
  placement_alloc place_alloc (std::max (logical_address (&data_end), KERNEL_VIRTUAL_BASE + multiboot_parser_.end ().value ()) << PAGE_SIZE, INITIAL_LOGICAL_LIMIT);
  
  frame_manager::initialize (multiboot_parser_.memory_map_begin (), multiboot_parser_.memory_map_end (), place_alloc);
  vm_manager::initialize (place_alloc.begin (), place_alloc.end ());
  
  exception_handler::install ();
  irq_handler::install ();
  trap_handler::install ();
  
  system_automaton::run (place_alloc.begin (), place_alloc.end ());
  kassert (0);
}
