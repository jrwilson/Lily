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
  ano_t action_number;
  const void* parameter;
  const void* value;
  size_t value_size;
  bd_t buffer;
  size_t buffer_size;
};

struct binding_count_args {
  uint32_t eip;
  ano_t action_number;
  const void* parameter;
};

struct create_args {
  uint32_t eip;
  bd_t buffer;
  size_t buffer_size;
  bool retain_privilege;
};

struct bind_args {
  uint32_t eip;
  aid_t output_automaton;
  ano_t output_action;
  const void* output_parameter;
  aid_t input_automaton;
  ano_t input_action;
  const void* input_parameter;
};

struct sbrk_args {
  uint32_t eip;
  size_t size;
};

struct buffer_create_args {
  uint32_t eip;
  size_t size;
};

struct buffer_map_args {
  uint32_t eip;
  bd_t buffer;
};

struct buffer_unmap_args {
  uint32_t eip;
  bd_t buffer;
};

struct map_args {
  uint32_t eip;
  const void* destination;
  const void* source;
  size_t size;
};

struct reserve_port_args {
  uint32_t eip;
  uint32_t port;
};

struct inb_args {
  uint32_t eip;
  uint32_t port;
};

struct outb_args {
  uint32_t eip;
  uint32_t port;
  uint32_t value;
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
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      rts::finish (current, ptr->action_number, ptr->parameter, ptr->value, ptr->value_size, ptr->buffer, ptr->buffer_size);
      return;
    }
    break;
  case LILY_SYSCALL_CREATE:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      create_args* ptr = reinterpret_cast<create_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (create_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<aid_t, int> r = rts::create (a, ptr->buffer, ptr->buffer_size, ptr->retain_privilege);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BIND:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      bind_args* ptr = reinterpret_cast<bind_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (bind_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bid_t, int> r = rts::bind (a, ptr->output_automaton, ptr->output_action, ptr->output_parameter, ptr->input_automaton, ptr->input_action, ptr->input_parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
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
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
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
      automaton* a = scheduler::current_action ().action->automaton;
      binding_count_args* ptr = reinterpret_cast<binding_count_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (binding_count_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<size_t, int> r = a->binding_count (ptr->action_number, ptr->parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_CREATE:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_create_args* ptr = reinterpret_cast<buffer_create_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_create_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_create (ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
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
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
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
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_unmap_args* ptr = reinterpret_cast<buffer_unmap_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_unmap_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<int, int> r = a->buffer_unmap (ptr->buffer);
      regs.eax = r.first;
      regs.ecx = r.second;
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
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<int, int> r = rts::map (a, ptr->destination, ptr->source, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
  case LILY_SYSCALL_RESERVE_PORT:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      reserve_port_args* ptr = reinterpret_cast<reserve_port_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (reserve_port_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<int, int> r = rts::reserve_port (a, ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
  case LILY_SYSCALL_INB:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      inb_args* ptr = reinterpret_cast<inb_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (inb_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<unsigned char, int> r = a->inb (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
  case LILY_SYSCALL_OUTB:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      outb_args* ptr = reinterpret_cast<outb_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (outb_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<unsigned char, int> r = a->outb (ptr->port, ptr->value);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
  case LILY_SYSCALL_SYSCONF:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      sysconf_args* ptr = reinterpret_cast<sysconf_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (sysconf_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
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
