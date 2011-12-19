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
  idt::set (TRAP_BASE + 0, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

extern "C" void
trap_dispatch (volatile registers regs)
{
  switch (regs.eax) {
  case system::FINISH:
    {
      const void* action_entry_point = reinterpret_cast<const void*> (regs.ebx);
      aid_t parameter = static_cast<aid_t> (regs.ecx);
      bool output_status = regs.edx;
      const void* buffer = reinterpret_cast<const void*> (regs.esi);

      if (action_entry_point != 0 || output_status) {
	automaton* current = scheduler::current_automaton ();
	
	if (action_entry_point != 0) {
	  // Check the action that was scheduled.
	  automaton::const_action_iterator pos = current->action_find (action_entry_point);
	  if (pos != current->action_end ()) {
	    scheduler::schedule (caction (current, *pos, parameter));
	  }
	  else {
	    // TODO:  Automaton scheduled a bad action.
	    kassert (0);
	  }
	}
	
	if (output_status) {
	  // Check the buffer.
	  size_t value_size = scheduler::current_value_size ();
	  if (value_size != 0 && !current->verify_span (buffer, value_size)) {
	    // TODO:  Automaton returned a bad buffer.
	    kassert (0);
	  }
	}
      }
      
      scheduler::finish (output_status, buffer);
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
      automaton* current_automaton = scheduler::current_automaton ();
      // The system automaton should not use interrupts to acquire logical address space.
      kassert (current_automaton != system_automaton::system_automaton);
      regs.eax = reinterpret_cast<uint32_t> (current_automaton->sbrk (size));
      return;
    }
    break;
  case system::BUFFER_CREATE:
    {
      regs.eax = rts::buffer_create (regs.ebx, scheduler::current_automaton ());
      return;
    }
    break;
  case system::BUFFER_SIZE:
    {
      regs.eax = rts::buffer_size (regs.ebx, scheduler::current_automaton ());
      return;
    }
    break;
  case system::BUFFER_INCREF:
    {
      regs.eax = rts::buffer_incref (regs.ebx, scheduler::current_automaton ());
      return;
    }
    break;
  case system::BUFFER_DECREF:
    {
      regs.eax = rts::buffer_decref (regs.ebx, scheduler::current_automaton ());
      return;
    }
    break;
  case system::BUFFER_MAP:
    {
      regs.eax = reinterpret_cast<uint32_t> (rts::buffer_map (regs.ebx, scheduler::current_automaton ()));
      return;
    }
    break;
  }
  
  // TODO:  Unknown system call.
  kassert (0);
}
