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
#include "idt.hpp"
#include "gdt.hpp"
#include "syscall_def.hpp"
#include "vm_def.hpp"
#include <utility>
#include "registers.hpp"
#include "aid.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"
#include "system_automaton.hpp"

using namespace std::rel_ops;

/* Operating system trap starts at 128. */
static const unsigned int TRAP_BASE = 128;

extern "C" void trap0 ();

void
trap_handler::install ()
{
  idt::set (TRAP_BASE + 0, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING3, descriptor_constants::PRESENT));
}

extern "C" void
trap_dispatch (volatile registers regs)
{
  switch (regs.eax) {
  case system::FINISH:
    {
      const void* action_entry_point = reinterpret_cast<const void*> (regs.ebx);
      aid_t parameter = static_cast<aid_t> (regs.ecx);
      bool status = regs.edx;
      bid_t bid = regs.esi;
      const void* buffer = reinterpret_cast<const void*> (regs.edi);

      const caction& current = scheduler::current_action ();
      
      if (action_entry_point != reinterpret_cast<const void*> (-1)) {
	// Check the action that was scheduled.
	automaton::const_action_iterator pos = current.automaton->action_find (action_entry_point);
	if (pos != current.automaton->action_end ()) {
	  scheduler::schedule (caction (current.automaton, *pos, (pos->parameter_mode == NO_PARAMETER) ? 0 : parameter));
	}
	else {
	  // TODO:  Automaton scheduled a bad action.
	  kassert (0);
	}
      }
      
      if (current.type == OUTPUT &&
	  status) {
	if (current.copy_value_mode == COPY_VALUE &&
	    !current.automaton->verify_span (buffer, current.copy_value_size)) {
	  // TODO:  Automaton returned a bad copy value.
	  kassert (0);
	}
	else if (current.buffer_value_mode == BUFFER_VALUE &&
		 !current.automaton->buffer_exists (bid)) {
	  // TODO:  Automaton returned a bad buffer.
	  kassert (0);
	}
      }
      
      scheduler::finish (status, bid, buffer);
      return;
    }
    break;
  case system::GETPAGESIZE:
    {
      regs.eax = PAGE_SIZE;
      return;
    }
    break;
  case system::SBRK:
    {
      ptrdiff_t size = regs.ebx;
      automaton* current_automaton = scheduler::current_action ().automaton;
      // The system automaton should not use interrupts to acquire logical address space.
      kassert (current_automaton != system_automaton::system_automaton);
      regs.eax = reinterpret_cast<uint32_t> (current_automaton->sbrk (size));
      return;
    }
    break;
  case system::BUFFER_CREATE:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_create (regs.ebx);
      return;
    }
    break;
  case system::BUFFER_CREATEB:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_create (regs.ebx, regs.ecx, regs.edx);
      return;
    }
    break;
  case system::BUFFER_APPEND:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_append (regs.ebx, regs.ecx);
      return;
    }
    break;
  case system::BUFFER_APPENDB:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_append (regs.ebx, regs.ecx, regs.edx, regs.esi);
      return;
    }
    break;
  case system::BUFFER_MAP:
    {
      regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().automaton->buffer_map (regs.ebx));
      return;
    }
    break;
  case system::BUFFER_DESTROY:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_destroy (regs.ebx);
      return;
    }
    break;
  case system::BUFFER_SIZE:
    {
      regs.eax = scheduler::current_action ().automaton->buffer_size (regs.ebx);
      return;
    }
    break;
  }
  
  // TODO:  Unknown system call.
  kassert (0);
}
