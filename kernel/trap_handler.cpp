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
  bool output_fired;
  bd_t bda;
  bd_t bdb;
};

struct create_args {
  uint32_t eip;
  bd_t text_bd;
  size_t text_size;
  bd_t bda;
  bd_t bdb;
  const char* name;
  size_t name_size;
  bool retain_privilege;
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

struct unbind_args {
  uint32_t eip;
  bid_t bid;
};

struct destroy_args {
  uint32_t eip;
  aid_t aid;
};

struct subscribe_unbound_args {
  uint32_t eip;
  bid_t bid;
  ano_t action_number;
};

struct unsubscribe_unbound_args {
  uint32_t eip;
  bid_t bid;
};

struct subscribe_destroyed_args {
  uint32_t eip;
  aid_t aid;
  ano_t action_number;
};

struct unsubscribe_destroyed_args {
  uint32_t eip;
  aid_t aid;
};

struct adjust_break_args {
  uint32_t eip;
  ptrdiff_t size;
};

struct buffer_create_args {
  uint32_t eip;
  size_t size;
};

struct buffer_copy_args {
  uint32_t eip;
  bd_t src;
};

struct buffer_resize_args {
  uint32_t eip;
  bd_t bd;
  size_t size;
};

struct buffer_assign_args {
  uint32_t eip;
  bd_t dst;
  bd_t src;
};

struct buffer_append_args {
  uint32_t eip;
  bd_t dst;
  bd_t src;
};

struct buffer_map_args {
  uint32_t eip;
  bd_t bd;
};

struct buffer_unmap_args {
  uint32_t eip;
  bd_t bd;
};

struct buffer_destroy_args {
  uint32_t eip;
  bd_t bd;
};

struct buffer_size_args {
  uint32_t eip;
  bd_t bd;
};

struct getmonotime_args {
  uint32_t eip;
  mono_time_t* t;
};

struct map_args {
  uint32_t eip;
  const void* destination;
  const void* source;
  size_t size;
};

struct unmap_args {
  uint32_t eip;
  const void* destination;
};

struct reserve_port_args {
  uint32_t eip;
  uint32_t port;
};

struct in_args {
  uint32_t eip;
  uint32_t port;
};

struct out_args {
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

struct unsubscribe_irq_args {
  uint32_t eip;
  int irq;
};

struct sysconf_args {
  uint32_t eip;
  int name;
};

struct lookup_args {
  uint32_t eip;
  const char* name;
  size_t size;
};

struct describe_args {
  uint32_t eip;
  aid_t aid;
};

struct syslog_args {
  uint32_t eip;
  const char* string;
  size_t size;
};

// The goal of this function is to demarshall system calls and dispatch.
extern "C" void
trap_dispatch (volatile registers regs)
{
  kassert (regs.number == SYSCALL_INTERRUPT);

  const shared_ptr<automaton>& a = scheduler::current_automaton ();

  /* These match the order in lily/syscall.h.
     Please keep it that way.
  */
  switch (regs.eax) {
  case LILY_SYSCALL_FINISH:
    {
      finish_args* ptr = reinterpret_cast<finish_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (finish_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      a->finish (a, ptr->action_number, ptr->parameter, ptr->output_fired, ptr->bda, ptr->bdb);
      return;
    }
    break;
  case LILY_SYSCALL_EXIT:
    {
      a->exit (a);
      return;
    }
    break;
  case LILY_SYSCALL_CREATE:
    {
      create_args* ptr = reinterpret_cast<create_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (create_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<aid_t, int> r = a->create (a, ptr->text_bd, ptr->text_size, ptr->bda, ptr->bdb, ptr->name, ptr->name_size, ptr->retain_privilege);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BIND:
    {
      bind_args* ptr = reinterpret_cast<bind_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (bind_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->bind (a, ptr->output_automaton, ptr->output_action, ptr->output_parameter, ptr->input_automaton, ptr->input_action, ptr->input_parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNBIND:
    {
      unbind_args* ptr = reinterpret_cast<unbind_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (unbind_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->unbind (a, ptr->bid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_DESTROY:
    {
      destroy_args* ptr = reinterpret_cast<destroy_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (destroy_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->destroy (a, ptr->aid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_UNBOUND:
    {
      subscribe_unbound_args* ptr = reinterpret_cast<subscribe_unbound_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (subscribe_unbound_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->subscribe_unbound (a, ptr->bid, ptr->action_number);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_UNBOUND:
    {
      unsubscribe_unbound_args* ptr = reinterpret_cast<unsubscribe_unbound_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (unsubscribe_unbound_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->unsubscribe_unbound (a, ptr->bid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_DESTROYED:
    {
      subscribe_destroyed_args* ptr = reinterpret_cast<subscribe_destroyed_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (subscribe_destroyed_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->subscribe_destroyed (a, ptr->aid, ptr->action_number);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_DESTROYED:
    {
      unsubscribe_destroyed_args* ptr = reinterpret_cast<unsubscribe_destroyed_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (unsubscribe_destroyed_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bid_t, int> r = a->unsubscribe_destroyed (a, ptr->aid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_ADJUST_BREAK:
    {
      adjust_break_args* ptr = reinterpret_cast<adjust_break_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (adjust_break_args))) {
	// BUG:  Can't get the arguments from the stack.
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
      buffer_create_args* ptr = reinterpret_cast<buffer_create_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_create_args))) {
	// BUG:  Can't get the arguments from the stack.
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
      buffer_copy_args* ptr = reinterpret_cast<buffer_copy_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_copy_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_copy (ptr->src);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_DESTROY:
    {
      buffer_destroy_args* ptr = reinterpret_cast<buffer_destroy_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_destroy_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->buffer_destroy (ptr->bd);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_SIZE:
    {
      buffer_size_args* ptr = reinterpret_cast<buffer_size_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_size_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->buffer_size (ptr->bd);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_RESIZE:
    {
      buffer_resize_args* ptr = reinterpret_cast<buffer_resize_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_resize_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_resize (ptr->bd, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_ASSIGN:
    {
      buffer_assign_args* ptr = reinterpret_cast<buffer_assign_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_assign_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->buffer_assign (ptr->dst, ptr->src);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_APPEND:
    {
      buffer_append_args* ptr = reinterpret_cast<buffer_append_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_append_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<bd_t, int> r = a->buffer_append (ptr->dst, ptr->src);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_MAP:
    {
      buffer_map_args* ptr = reinterpret_cast<buffer_map_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_map_args))) {
	// BUG:  Can't get the arguments from the stack.
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
      buffer_unmap_args* ptr = reinterpret_cast<buffer_unmap_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (buffer_unmap_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->buffer_unmap (ptr->bd);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SYSCONF:
    {
      sysconf_args* ptr = reinterpret_cast<sysconf_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (sysconf_args))) {
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
    break;
  case LILY_SYSCALL_LOOKUP:
    {
      lookup_args* ptr = reinterpret_cast<lookup_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (lookup_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->lookup (ptr->name, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_DESCRIBE:
    {
      describe_args* ptr = reinterpret_cast<describe_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (describe_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->describe (ptr->aid);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_GETAID:
    {
      regs.eax = a->aid ();
      regs.ecx = LILY_SYSCALL_ESUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETINITA:
    {
      regs.eax = a->inita ();
      regs.ecx = LILY_SYSCALL_ESUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETINITB:
    {
      regs.eax = a->initb ();
      regs.ecx = LILY_SYSCALL_ESUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETMONOTIME:
    {
      getmonotime_args* ptr = reinterpret_cast<getmonotime_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (getmonotime_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->getmonotime (ptr->t);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_MAP:
    {
      map_args* ptr = reinterpret_cast<map_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (map_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->map (ptr->destination, ptr->source, ptr->size);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNMAP:
    {
      unmap_args* ptr = reinterpret_cast<unmap_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (unmap_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->unmap (ptr->destination);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_RESERVE_PORT:
    {
      reserve_port_args* ptr = reinterpret_cast<reserve_port_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (reserve_port_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->reserve_port (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNRESERVE_PORT:
    {
      reserve_port_args* ptr = reinterpret_cast<reserve_port_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (reserve_port_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->unreserve_port (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INB:
    {
      in_args* ptr = reinterpret_cast<in_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (in_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<uint8_t, int> r = a->inb (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTB:
    {
      out_args* ptr = reinterpret_cast<out_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (out_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->outb (ptr->port, ptr->value);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INW:
    {
      in_args* ptr = reinterpret_cast<in_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (in_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<uint16_t, int> r = a->inw (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTW:
    {
      out_args* ptr = reinterpret_cast<out_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (out_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->outw (ptr->port, ptr->value);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INL:
    {
      in_args* ptr = reinterpret_cast<in_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (in_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<uint32_t, int> r = a->inl (ptr->port);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTL:
    {
      out_args* ptr = reinterpret_cast<out_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (out_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->outl (ptr->port, ptr->value);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_IRQ:
    {
      subscribe_irq_args* ptr = reinterpret_cast<subscribe_irq_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (subscribe_irq_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->subscribe_irq (a, ptr->irq, ptr->action_number, ptr->parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_IRQ:
    {
      unsubscribe_irq_args* ptr = reinterpret_cast<unsubscribe_irq_args*> (regs.useresp);
      if (!a->verify_stack (ptr, sizeof (unsubscribe_irq_args))) {
	// BUG:  Can't get the arguments from the stack.
	kassert (0);
      }
      pair<int, int> r = a->unsubscribe_irq (ptr->irq);
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
