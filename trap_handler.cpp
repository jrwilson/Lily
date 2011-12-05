/*
  File
  ----
  trap_handler.hpp
  
  Description
  -----------
  Object for handling traps.
  
  Authors:
  Justin R. Wilson
*/

#include "trap_handler.hpp"
#include "interrupt_descriptor_table.hpp"
#include "global_descriptor_table.hpp"
#include "syscall_def.hpp"
#include "vm_def.hpp"
#include "system_automaton.hpp"
#include <utility>

using namespace std::rel_ops;

/* Operating system trap starts at 128. */
static const interrupt_number TRAP_BASE = 128;

extern "C" void trap0 ();

void
trap_handler::install ()
{
  interrupt_descriptor_table::set (TRAP_BASE + 0, make_trap_gate (trap0, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

extern "C" void
trap_dispatch (registers regs)
{
  syscall_t syscall = extract_syscall (regs.eax);
  
  switch (syscall) {
  case SYSCALL_FINISH:
    {
      local_func const action_entry_point = reinterpret_cast<local_func> (regs.ebx);
      void* const parameter = reinterpret_cast<void*> (regs.ecx);
      void* const buffer = reinterpret_cast<void*> (regs.edx);
      system_automaton::finish_action (is_schedule_valid (regs.eax), action_entry_point, parameter, is_output_valid (regs.eax), buffer);
      return;
    }
    break;
  case SYSCALL_GET_PAGE_SIZE:
    {
      regs.eax = SYSERROR_SUCCESS;
      regs.ebx = PAGE_SIZE;
      return;
    }
    break;
  case SYSCALL_ALLOCATE:
    {
      size_t size = regs.ebx;
      if (size > 0) {
      	logical_address ptr = system_automaton::alloc (size);
      	if (ptr != logical_address ()) {
      	  regs.eax = SYSERROR_SUCCESS;
      	  regs.ebx = reinterpret_cast<uint32_t> (ptr.value ());
      	}
      	else {
      	  regs.eax = SYSERROR_OUT_OF_MEMORY;
      	  regs.ebx = 0;
      	}
      }
      else {
      	// Zero size.
      	regs.eax = SYSERROR_SUCCESS;
      	regs.ebx = 0;
      }
      return;
    }
    break;
  }
  
  system_automaton::unknown_system_call ();
}
