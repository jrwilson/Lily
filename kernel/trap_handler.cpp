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
  uint32_t eip;
  const void* action_entry_point;
  const void* parameter;
  const void* value;
  size_t value_size;
  bid_t buffer;
  size_t buffer_size;
};

struct sbrk_args {
  uint32_t eip;
  size_t size;
};

struct buffer_map_args {
  uint32_t eip;
  bid_t buffer;
};

struct map_args {
  uint32_t eip;
  const void* destination;
  const void* source;
  size_t size;
};

struct sysconf_args {
  uint32_t eip;
  int name;
};

// The goal of this function is to demarshall system calls and dispatch.
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
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      rts::finish (current, ptr->action_entry_point, ptr->parameter, ptr->value, ptr->value_size, ptr->buffer, ptr->buffer_size);
      return;
    }
    break;
  case LILY_SYSCALL_CREATE:
    {
      // BUG
      kassert (0);
      const void* automaton_buffer = reinterpret_cast<const void*> (regs.ebx);
      size_t automaton_size = regs.ecx;
      
      const caction& current = scheduler::current_action ();
      
      if (current.action->automaton->verify_span (automaton_buffer, automaton_size)) {
	regs.eax = rts::create (automaton_buffer, automaton_size);
	return;
      }
      else {
	// BUG:  The automaton specified a bad buffer.
	kassert (0);
      }
    }
    break;
  case LILY_SYSCALL_BIND:
    {
      // BUG
      kassert (0);
      rts::bind ();
      return;
    }
    break;
  case LILY_SYSCALL_LOOSE:
    {
      // BUG
      kassert (0);
      rts::loose ();
  	return;
    }
    break;
  case LILY_SYSCALL_DESTROY:
    {
      // BUG
      kassert (0);
      rts::destroy ();
      return;
    }
    break;
  case LILY_SYSCALL_SBRK:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      sbrk_args* ptr = reinterpret_cast<sbrk_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (sbrk_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<void*, int> r = a->sbrk (ptr->size);
      regs.eax = reinterpret_cast<uint32_t> (r.first);
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BINDING_COUNT:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->binding_count (reinterpret_cast<const void*> (regs.ebx), reinterpret_cast<const void*> (regs.ecx));
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_CREATE:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_create (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_COPY:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_copy (regs.ebx, regs.ecx, regs.edx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_GROW:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_grow (regs.ebx, regs.ecx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_APPEND:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_append (regs.ebx, regs.ecx, regs.edx, regs.esi);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_ASSIGN:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_assign (regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_MAP:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_map_args* ptr = reinterpret_cast<buffer_map_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_map_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<void*, int> r = a->buffer_map (ptr->buffer);
      regs.eax = reinterpret_cast<uint32_t> (r.first);
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_UNMAP:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_unmap (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_DESTROY:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_destroy (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_SIZE:
    {
      // BUG
      kassert (0);
      regs.eax = scheduler::current_action ().action->automaton->buffer_size (regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_MAP:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      map_args* ptr = reinterpret_cast<map_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (map_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = rts::map (a, ptr->destination, ptr->source, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
  case LILY_SYSCALL_SYSCONF:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      sysconf_args* ptr = reinterpret_cast<sysconf_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (sysconf_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      switch (ptr->name) {
      case LILY_SYSCALL_SYSCONF_PAGESIZE:
	regs.eax = PAGE_SIZE;
	regs.ecx = LILY_SYSCALL_ESUCCESS;
	return;
	break;
      default:
	regs.eax = 0;
	regs.ecx = LILY_SYSCALL_EINVAL;
	return;
	break;
      }
      return;
    }
  default:
    // BUG:  Unknown system call.
    kassert (0);
  }
}
