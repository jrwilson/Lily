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

using namespace std::rel_ops;

// User operating system trap use interrupt 0x80.
// Privileged operations use interrupt 0x81.
static const unsigned int SYSCALL_INTERRUPT = 0x80;
static const unsigned int PRIVCALL_INTERRUPT = 0x81;

extern "C" void trap0 ();
extern "C" void trap1 ();

void
trap_handler::install ()
{
  idt::set (SYSCALL_INTERRUPT, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
  idt::set (PRIVCALL_INTERRUPT, make_trap_gate (trap1, gdt::KERNEL_CODE_SELECTOR, descriptor::RING3, descriptor::PRESENT));
}

extern "C" void
trap_dispatch (volatile registers regs)
{
  switch (regs.number) {
  case SYSCALL_INTERRUPT:
    switch (regs.eax) {
    case syscall::FINISH:
      {
	const void* action_entry_point = reinterpret_cast<const void*> (regs.ebx);
	aid_t parameter = static_cast<aid_t> (regs.ecx);
	bool status = regs.edx;
	bid_t bid = regs.esi;
	const void* buffer = reinterpret_cast<const void*> (regs.edi);
	
	const caction& current = scheduler::current_action ();
	
	if (action_entry_point != 0) {
	  // Check the action that was scheduled.
	  automaton::const_action_iterator pos = current.action->automaton->action_find (action_entry_point);
	  if (pos != current.action->automaton->action_end ()) {
	    // Ignore the parameter if NO_PARAMETER.
	    scheduler::schedule (caction (*pos, ((*pos)->parameter_mode == NO_PARAMETER) ? 0 : parameter));
	  }
	  else {
	    // TODO:  Automaton scheduled a bad action.
	    kassert (0);
	  }
	}
	
	if (current.action->type == OUTPUT &&
	    status) {
	  if (current.action->copy_value_mode == COPY_VALUE &&
	      !current.action->automaton->verify_span (buffer, current.action->copy_value_size)) {
	    // TODO:  Automaton returned a bad copy value.
	    kassert (0);
	  }
	  else if (current.action->buffer_value_mode == BUFFER_VALUE &&
		   !current.action->automaton->buffer_exists (bid)) {
	    // TODO:  Automaton returned a bad buffer.
	    kassert (0);
	  }
	}
	
	scheduler::finish (status, bid, buffer);
	return;
      }
      break;
    case syscall::GETPAGESIZE:
      {
	regs.eax = PAGE_SIZE;
	return;
      }
      break;
    case syscall::SBRK:
      {
	regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->sbrk (regs.ebx));
	return;
      }
      break;
    case syscall::BINDING_COUNT:
      {
	regs.eax = scheduler::current_action ().action->automaton->binding_count (reinterpret_cast<const void*> (regs.ebx), regs.ecx);
	return;
      }
      break;
    case syscall::BUFFER_CREATE:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_create (regs.ebx);
	return;
      }
      break;
    case syscall::BUFFER_COPY:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_copy (regs.ebx, regs.ecx, regs.edx);
	return;
      }
      break;
    case syscall::BUFFER_GROW:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_grow (regs.ebx, regs.ecx);
	return;
      }
      break;
    case syscall::BUFFER_APPEND:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_append (regs.ebx, regs.ecx, regs.edx, regs.esi);
	return;
      }
      break;
    case syscall::BUFFER_ASSIGN:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_assign (regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);
	return;
      }
      break;
    case syscall::BUFFER_MAP:
      {
	regs.eax = reinterpret_cast<uint32_t> (scheduler::current_action ().action->automaton->buffer_map (regs.ebx));
	return;
      }
      break;
    case syscall::BUFFER_DESTROY:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_destroy (regs.ebx);
	return;
      }
      break;
    case syscall::BUFFER_SIZE:
      {
	regs.eax = scheduler::current_action ().action->automaton->buffer_size (regs.ebx);
	return;
      }
      break;
    }
    break;
  case PRIVCALL_INTERRUPT:
    if (scheduler::current_action ().action->automaton->can_execute_privileged ()) {
      switch (regs.eax) {
      case privcall::INVLPG:
	{
	  asm ("invlpg (%0)\n" :: "r"(regs.ebx));
	  return;
	}
	break;
      }
    }
    else {
      // TODO:  Unprivileged automaton tried to execute privileged instruction.
      kassert (0);
    }
    break;
  }

  // TODO:  Unknown system call.
  kassert (0);
}
