#ifndef __syscall_handler_hpp__
#define __syscall_handler_hpp__

/*
  File
  ----
  syscall_handler.hpp
  
  Description
  -----------
  Declarations for functions to manage system calls.

  Authors:
  Justin R. Wilson
*/

#include "interrupt_descriptor_table.hpp"
#include "syscall_def.hpp"
#include "scheduler.hpp"
#include <utility>

using namespace std::rel_ops;

template <class AllocatorTag, template <typename> class Allocator>
class syscall_handler {
private:
  static void
  handler (registers* regs)
  {
    syscall_t syscall = static_cast<syscall_t> (EXTRACT_SYSCALL (regs->eax));
    
    switch (syscall) {
    case SYSCALL_FINISH:
      {
	int output_status = IS_OUTPUT_VALID (regs->eax);
	value_t output_value = regs->edx;
	scheduler<AllocatorTag, Allocator>::finish_action (output_status, output_value);
	return;
      }
      break;
    case SYSCALL_SCHEDULE:
      {
	void* action_entry_point = (void*)regs->ebx;
	parameter_t parameter = regs->ecx;
	int output_status = IS_OUTPUT_VALID (regs->eax);
	value_t output_value = regs->edx;
	scheduler<AllocatorTag, Allocator>::schedule_action (scheduler<AllocatorTag, Allocator>::get_current_automaton (), action_entry_point, parameter);
	scheduler<AllocatorTag, Allocator>::finish_action (output_status, output_value);
	return;
      }
      break;
    case SYSCALL_GET_PAGE_SIZE:
      {
	regs->eax = SYSERROR_SUCCESS;
	regs->ebx = PAGE_SIZE;
	return;
      }
      break;
    case SYSCALL_ALLOCATE:
      {
	size_t size = regs->ebx;
	if (size > 0) {
	  logical_address ptr = scheduler<AllocatorTag, Allocator>::get_current_automaton ()->alloc (size);
	  if (ptr != logical_address ()) {
	    regs->eax = SYSERROR_SUCCESS;
	    regs->ebx = reinterpret_cast<uint32_t> (ptr.value ());
	  }
	  else {
	    regs->eax = SYSERROR_OUT_OF_MEMORY;
	    regs->ebx = 0;
	  }
	}
	else {
	  // Zero size.
	  regs->eax = SYSERROR_SUCCESS;
	  regs->ebx = 0;
	}
	return;
      }
      break;
    }
    
    /* TODO:  Unknown syscall. */  
    kputs ("Unknown syscall\n");
    halt ();
  }
  
  
public:
  static void
  initialize ()
  {
    set_interrupt_handler (TRAP_BASE + 0, handler);
  }
};


#endif /* __syscall_handler_hpp__ */
