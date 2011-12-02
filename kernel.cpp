/*
  File
  ----
  kernel.cpp
  
  Description
  -----------
  The kernel object.

  Authors:
  Justin R. Wilson
*/

#include "kernel.hpp"
#include "vm_def.hpp"

/* logical_end denotes the end of the logical address space as setup by loader.s.
   The multiboot data structures use physical addresses.
   loader.s identity maps up to (logical_address - KERNEL_VIRTUAL_BASE) so they can be read without manual translation.
 */

extern uint32_t multiboot_magic;
extern multiboot_info_t* multiboot_info;
extern void* logical_end;

extern int data_end;

kernel::kernel () :
  multiboot_parser_ (multiboot_magic, multiboot_info),
  placement_alloc_ (std::max (logical_address (&data_end) << PAGE_SIZE,
			      (logical_address (KERNEL_VIRTUAL_BASE) + multiboot_parser_.end ().value ()) << PAGE_SIZE),
		    logical_address (logical_end)),
  frame_manager_ (multiboot_parser_.memory_map_begin (),
		  multiboot_parser_.memory_map_end (),
		  placement_alloc_),
  vm_manager_ (placement_alloc_.begin (), placement_alloc_.end (), frame_manager_)
{ }
