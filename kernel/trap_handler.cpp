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
#include "registers.hpp"
#include "kassert.hpp"
#include "action.hpp"
#include "scheduler.hpp"
#include "rts.hpp"
#include <lily/syscall.h>

// Operating system traps use interrupt 0x80.
static const unsigned int SYSCALL_INTERRUPT = 0x80;

extern "C" void trap0 ();

void
trap_handler::install ()
{
  idt::set (SYSCALL_INTERRUPT, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
}

struct finish_args {
  uint32_t ebp;
  uint32_t eip;
  const void* action_entry_point;
  const void* parameter;
  const void* value;
  size_t value_size;
  bid_t buffer;
  size_t buffer_size;
};

extern "C" void
trap_dispatch (volatile registers regs)
{
  kassert (regs.number == SYSCALL_INTERRUPT);
  switch (regs.eax) {
  case LILY_SYSCALL_FINISH:
    {
      const caction& current = scheduler::current_action ();
      finish_args* ptr = reinterpret_cast<finish_args*> (regs.useresp);
      if (!current.action->automaton->verify_span (ptr, sizeof (finish_args))) {
	// TODO:  Can't get the arguments from the stack.
	kassert (0);
      }
      
      if (ptr->action_entry_point != 0) {
  	// Check the action that was scheduled.
  	const paction* action = current.action->automaton->find_action (ptr->action_entry_point);
  	if (action != 0) {
  	  scheduler::schedule (caction (action, ptr->parameter));
  	}
  	else {
  	  // TODO:  Automaton scheduled a bad action.
  	  kout << "aep = " << hexformat (ptr->action_entry_point) << endl;
  	  kassert (0);
  	}
      }

      if (current.action->type == OUTPUT) {
  	// Check the data produced by an output.
  	if (ptr->value != 0) {
  	  if (ptr->value_size > LILY_LIMITS_MAX_VALUE_SIZE || (ptr->value_size != 0 && !current.action->automaton->verify_span (ptr->value, ptr->value_size))) {
  	    // TODO:  The automaton produced a bad copy value.
  	    kassert (0);
  	  }
  	}
  	else {
  	  // No copy value was produced.
  	  ptr->value = 0;
  	  ptr->value_size = 0;
  	}

  	const size_t s = current.action->automaton->buffer_size (ptr->buffer);
  	if (s != static_cast<size_t> (-1)) {
  	  if (ptr->buffer_size > s) {
  	    // TODO:  The action produced a value buffer but claims the size is larger than the size in memory.
  	    kassert (0);
  	  }
  	}
  	else {
  	  // No buffer was produced.
  	  ptr->buffer = -1;
  	  ptr->buffer_size = 0;
  	}
      }

      scheduler::finish (ptr->value, ptr->value_size, ptr->buffer, ptr->buffer_size);
      return;
    }
    break;
  case LILY_SYSCALL_CREATE:
    {
      const void* automaton_buffer = reinterpret_cast<const void*> (regs.ebx);
      size_t automaton_size = regs.ecx;
      
      const caction& current = scheduler::current_action ();
      
      if (current.action->automaton->verify_span (automaton_buffer, automaton_size)) {
	regs.eax = rts::create (automaton_buffer, automaton_size);
	return;
      }
      else {
	// TODO:  The automaton specified a bad buffer.
	kassert (0);
      }
    }
    break;
  case LILY_SYSCALL_BIND:
    {
      rts::bind ();
      return;
    }
    break;
  case LILY_SYSCALL_LOOSE:
    {
      rts::loose ();
  	return;
    }
    break;
  case LILY_SYSCALL_DESTROY:
    {
      rts::destroy ();
      return;
    }
    break;
  case LILY_SYSCALL_SBRK:
    {
      regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->sbrk (regs.ebx));
      return;
    }
      break;
  case LILY_SYSCALL_BINDING_COUNT:
    {
      regs.eax = scheduler::current_action ().action->automaton->binding_count (reinterpret_cast<const void*> (regs.ebx), reinterpret_cast<const void*> (regs.ecx));
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_CREATE:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_create (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_COPY:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_copy (regs.ebx, regs.ecx, regs.edx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_GROW:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_grow (regs.ebx, regs.ecx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_APPEND:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_append (regs.ebx, regs.ecx, regs.edx, regs.esi);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_ASSIGN:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_assign (regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_MAP:
    {
      regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->buffer_map (regs.ebx));
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_UNMAP:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_unmap (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_DESTROY:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_destroy (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_SIZE:
    {
      regs.eax = scheduler::current_action ().action->automaton->buffer_size (regs.ebx);
      return;
    }
    break;
  }

  // TODO:  Unknown system call.
  kassert (0);
}
