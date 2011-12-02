#ifndef __kernel_hpp__
#define __kernel_hpp__

/*
  File
  ----
  kernel.hpp
  
  Description
  -----------
  The kernel object.

  Authors:
  Justin R. Wilson
*/

#include "welcome_message.hpp"
#include "global_descriptor_table.hpp"
#include "interrupt_descriptor_table.hpp"
#include "multiboot_parser.hpp"
#include "placement_allocator.hpp"
#include "frame_manager.hpp"
#include "vm_manager.hpp"

struct kernel {
  welcome_message welcome_message_;
  global_descriptor_table global_descriptor_table_;
  interrupt_descriptor_table interrupt_descriptor_table_;
  multiboot_parser multiboot_parser_;
  placement_alloc placement_alloc_;
  frame_manager frame_manager_;
  vm_manager vm_manager_;

  kernel ();
};

#endif /* __kernel_hpp__ */
