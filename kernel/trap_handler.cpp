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
#include "vm_def.hpp"
#include <utility>
#include "registers.hpp"
#include "aid.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"
#include "syscall.hpp"
#include "system_automaton.hpp"

using namespace std::rel_ops;

// The interrupt for finishing an action is 0x80.
// Operating system traps use interrupt 0x81.
// Certain operations of the system automaton use interrupt 0x82.

static const unsigned int FINISH_INTERRUPT = 0x80;
static const unsigned int SYSCALL_INTERRUPT = 0x81;
static const unsigned int SACALL_INTERRUPT = 0x82;

extern "C" void trap0 ();
extern "C" void trap1 ();
extern "C" void trap2 ();

void
trap_handler::install ()
{
  idt::set (FINISH_INTERRUPT, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
  idt::set (SYSCALL_INTERRUPT, make_trap_gate (trap1, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
  idt::set (SACALL_INTERRUPT, make_trap_gate (trap2, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
}

extern "C" void
trap_dispatch (volatile registers regs)
{
  switch (regs.number) {
  case FINISH_INTERRUPT:
    {
      const void* const action_entry_point = reinterpret_cast<const void*> (regs.eax);
      aid_t const parameter = static_cast<aid_t> (regs.ebx);
      const void* copy_value = reinterpret_cast<const void*> (regs.ecx);
      size_t copy_value_size = regs.edx;
      bid_t buffer = regs.esi;
      size_t buffer_size = regs.edi;
      
      const caction& current = scheduler::current_action ();
      
      if (action_entry_point != 0) {
	// Check the action that was scheduled.
	automaton::const_action_iterator pos = current.action->automaton->action_find (action_entry_point);
	if (pos != current.action->automaton->action_end ()) {
	  scheduler::schedule (caction (*pos, parameter));
	}
	else {
	  // TODO:  Automaton scheduled a bad action.
	  kassert (0);
	}
      }

      if (current.action->type == OUTPUT) {
	// Check the data produced by an output.
	if (copy_value != 0) {
	  if (copy_value_size > MAX_COPY_VALUE_SIZE || (copy_value_size != 0 && !current.action->automaton->verify_span (copy_value, copy_value_size))) {
	    // TODO:  The automaton produced a bad copy value.
	    kassert (0);
	  }
	}
	else {
	  // No copy value was produced.
	  copy_value = 0;
	  copy_value_size = 0;
	}

	const size_t s = current.action->automaton->buffer_size (buffer);
	if (s != static_cast<size_t> (-1)) {
	  if (buffer_size > s) {
	    // TODO:  The action produced a value buffer but claims the size is larger than the size in memory.
	    kassert (0);
	  }
	}
	else {
	  // No buffer was produced.
	  buffer = -1;
	  buffer_size = 0;
	}
      }
      
      scheduler::finish (copy_value, copy_value_size, buffer, buffer_size);
      return;
    }
    break;
  case SYSCALL_INTERRUPT:
    switch (regs.eax) {
    case lilycall::GETPAGESIZE:
      {
	regs.eax = PAGE_SIZE;
	return;
      }
      break;
    case lilycall::SBRK:
      {
	regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->sbrk (regs.ebx));
	return;
      }
      break;
    case lilycall::BINDING_COUNT:
      {
	regs.eax = scheduler::current_action ().action->automaton->binding_count (reinterpret_cast<const void*> (regs.ebx), regs.ecx);
	return;
      }
      break;
    case lilycall::BUFFER_CREATE:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_create (regs.ebx);
	return;
      }
      break;
    case lilycall::BUFFER_COPY:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_copy (regs.ebx, regs.ecx, regs.edx);
	return;
      }
      break;
    case lilycall::BUFFER_GROW:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_grow (regs.ebx, regs.ecx);
	return;
      }
      break;
    case lilycall::BUFFER_APPEND:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_append (regs.ebx, regs.ecx, regs.edx, regs.esi);
	return;
      }
      break;
    case lilycall::BUFFER_ASSIGN:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_assign (regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);
	return;
      }
      break;
    case lilycall::BUFFER_MAP:
      {
	regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->buffer_map (regs.ebx));
	return;
      }
      break;
    case lilycall::BUFFER_DESTROY:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_destroy (regs.ebx);
	return;
      }
      break;
    case lilycall::BUFFER_SIZE:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_size (regs.ebx);
	return;
      }
      break;
    }
    break;
  case SACALL_INTERRUPT:
    if (scheduler::current_action ().action->automaton == system_automaton::instance) {
      switch (regs.eax) {
      case sacall::INVLPG:
	{
	  asm ("invlpg (%0)\n" :: "r"(regs.ebx));
	  return;
	}
	break;
      }
    }
    else {
      // An automaton other than the system automaton tried to execute a special instruction.
      kassert (0);
    }
    break;
  }

  // TODO:  Unknown system call.
  kassert (0);
}
