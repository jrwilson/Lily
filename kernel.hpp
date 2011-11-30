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
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "multiboot_parser.hpp"
#include "placement_allocator.hpp"
#include "frame_manager.hpp"
#include "vm_manager.hpp"
#include "vm_def.hpp"

extern int data_end;

class kernel {
private:
  welcome_message wm_;
  global_descriptor_table gdt_;
  multiboot_parser multiboot_parser_;
  placement_alloc placement_alloc_;
  frame_manager frame_manager_;
  vm_manager vm_manager_;
  interrupt_descriptor_table idt_;
  exception_handler exception_handler_;
  irq_handler irq_handler_;
  trap_handler trap_handler_;
  system_automaton system_automaton_;

public:
  kernel (uint32_t multiboot_magic,
	  multiboot_info_t* multiboot_info,
	  void* logical_end) :
    multiboot_parser_ (multiboot_magic, multiboot_info),
    placement_alloc_ (std::max (logical_address (&data_end) << PAGE_SIZE,
				(KERNEL_VIRTUAL_BASE + multiboot_parser_.end ().value ()) << PAGE_SIZE),
		      logical_address (logical_end)),
    frame_manager_ (multiboot_parser_.memory_map_begin (),
		    multiboot_parser_.memory_map_end (),
		    placement_alloc_),
    vm_manager_ (placement_alloc_.begin (), placement_alloc_.end (), frame_manager_),
    exception_handler_ (idt_, system_automaton_),
    irq_handler_ (idt_),
    trap_handler_ (idt_, system_automaton_),
    system_automaton_ (frame_manager_, vm_manager_, placement_alloc_.begin (), placement_alloc_.end ())
  { }

  void
  run (void)
  {
    system_automaton_.run ();
  }
};

#endif /* __kernel_hpp__ */
