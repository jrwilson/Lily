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
  int parameter;
  bd_t bd;
  int flags;
};

struct create_args {
  uint32_t eip;
  bd_t text_bd;
  size_t text_buffer_size;
  bool retain_privilege;
  bd_t data_bd;
};

struct bind_args {
  uint32_t eip;
  aid_t output_automaton;
  ano_t output_action;
  int output_parameter;
  aid_t input_automaton;
  ano_t input_action;
  int input_parameter;
};

struct subscribe_destroyed_args {
  uint32_t eip;
  aid_t aid;
};

struct adjust_break_args {
  uint32_t eip;
  size_t size;
};

struct buffer_create_args {
  uint32_t eip;
  size_t size;
};

struct buffer_copy_args {
  uint32_t eip;
  bd_t src;
  size_t offset;
  size_t length;
};

struct buffer_grow_args {
  uint32_t eip;
  bd_t bd;
  size_t size;
};

struct buffer_append_args {
  uint32_t eip;
  bd_t dst;
  bd_t src;
  size_t offset;
  size_t length;
};

struct buffer_map_args {
  uint32_t eip;
  bd_t bd;
};

struct buffer_unmap_args {
  uint32_t eip;
  bd_t bd;
};

struct buffer_capacity_args {
  uint32_t eip;
  bd_t bd;
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

struct subscribe_irq_args {
  uint32_t eip;
  int irq;
  ano_t action_number;
  int parameter;
};

struct sysconf_args {
  uint32_t eip;
  int name;
};

struct syslog_args {
  uint32_t eip;
  char* string;
  size_t size;
};

// The goal of this function is to demarshall system calls and dispatch.
extern "C" void
trap_dispatch (volatile registers regs)
{
  kassert (regs.number == SYSCALL_INTERRUPT);

  /* These match the order in lily/syscall.h.
     Please keep it that way.
  */
  switch (regs.eax) {
  case LILY_SYSCALL_FINISH:
    {
      const caction& current = scheduler::current_action ();
      finish_args* ptr = reinterpret_cast<finish_args*> (regs.useresp);
      if (!current.action->automaton->verify_span (ptr, sizeof (finish_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      rts::finish (current, ptr->action_number, ptr->parameter, ptr->bd, ptr->flags);
      return;
    }
    break;
  case LILY_SYSCALL_EXIT:
    {
      // BUG
      kassert (0);
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
      pair<aid_t, int> r = rts::create (a, ptr->text_bd, ptr->text_buffer_size, ptr->retain_privilege, ptr->data_bd);
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
  case LILY_SYSCALL_UNBIND:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_DESTROY:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_UNBOUND:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_UNBOUND:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_DESTROYED:
    {
    automaton* a = scheduler::current_action ().action->automaton;
      subscribe_destroyed_args* ptr = reinterpret_cast<subscribe_destroyed_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (subscribe_destroyed_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bid_t, int> r = rts::subscribe_destroyed (a, ptr->aid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_DESTROYED:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_ADJUST_BREAK:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      adjust_break_args* ptr = reinterpret_cast<adjust_break_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (adjust_break_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<void*, int> r = a->adjust_break (ptr->size);
      regs.eax = reinterpret_cast<uint32_t> (r.first);
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
    automaton* a = scheduler::current_action ().action->automaton;
      buffer_copy_args* ptr = reinterpret_cast<buffer_copy_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_copy_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_copy (ptr->src, ptr->offset, ptr->length);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_GROW:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_grow_args* ptr = reinterpret_cast<buffer_grow_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_grow_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_grow (ptr->bd, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_APPEND:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_append_args* ptr = reinterpret_cast<buffer_append_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_append_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_append (ptr->dst, ptr->src, ptr->offset, ptr->length);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_ASSIGN:
    {
      // BUG
      kassert (0);
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
      pair<void*, int> r = a->buffer_map (ptr->bd);
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
      pair<int, int> r = a->buffer_unmap (ptr->bd);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_DESTROY:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_CAPACITY:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      buffer_capacity_args* ptr = reinterpret_cast<buffer_capacity_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (buffer_capacity_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<int, int> r = a->buffer_capacity (ptr->bd);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
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
    break;
  case LILY_SYSCALL_SET_REGISTRY:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_GET_REGISTRY:
    {
      // BUG
      kassert (0);
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
    break;
  case LILY_SYSCALL_UNMAP:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
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
    break;
  case LILY_SYSCALL_UNRESERVE_PORT:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
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
    break;
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
    break;
  case LILY_SYSCALL_SUBSCRIBE_IRQ:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      subscribe_irq_args* ptr = reinterpret_cast<subscribe_irq_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (subscribe_irq_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<unsigned char, int> r = a->subscribe_irq (ptr->irq, ptr->action_number, ptr->parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_IRQ:
    {
      // BUG
      kassert (0);
      return;
    }
    break;
  case LILY_SYSCALL_SYSLOG:
    {
      automaton* a = scheduler::current_action ().action->automaton;
      syslog_args* ptr = reinterpret_cast<syslog_args*> (regs.useresp);
      if (!a->verify_span (ptr, sizeof (syslog_args))) {
	// BUG:  Can't get the arguments from the stack.  Don't use verify_span!  Use verify_stack!
	kassert (0);
      }
      pair<unsigned char, int> r = a->syslog (ptr->string, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  default:
    // BUG:  Unknown system call.
    kassert (0);
    break;
  }
}
