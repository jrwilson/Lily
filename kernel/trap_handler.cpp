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

struct create_args {
  bd_t text_bd;
  size_t text_size;
  bd_t bda;
  bd_t bdb;
  bool retain_privilege;
};

struct bind_args {
  aid_t output_automaton;
  ano_t output_action;
  int output_parameter;
  aid_t input_automaton;
  ano_t input_action;
  int input_parameter;
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
  case LILY_SYSCALL_SCHEDULE:
    {
      pair<int, lily_error_t> r = a->schedule (a, regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_FINISH:
    {
      scheduler::finish (regs.ebx, regs.ecx, regs.edx);
      return;
    }
    break;
  case LILY_SYSCALL_EXIT:
    {
      a->exit (a, regs.ebx);
      return;
    }
    break;
  case LILY_SYSCALL_CREATE:
    {
      create_args* ptr = reinterpret_cast<create_args*> (regs.ebx);
      if (!a->verify_stack (ptr, sizeof (create_args))) {
	kpanic ("TODO:  Can't get create arguments from the stack");
      }
      pair<aid_t, lily_error_t> r = a->create (a, ptr->text_bd, ptr->text_size, ptr->bda, ptr->bdb, ptr->retain_privilege);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BIND:
    {
      bind_args* ptr = reinterpret_cast<bind_args*> (regs.ebx);
      if (!a->verify_stack (ptr, sizeof (bind_args))) {
	kpanic ("TODO:  Can't get bind arguments from the stack");
      }
      pair<bid_t, lily_error_t> r = a->bind (a, ptr->output_automaton, ptr->output_action, ptr->output_parameter, ptr->input_automaton, ptr->input_action, ptr->input_parameter);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNBIND:
    {
      pair<int, lily_error_t> r = a->unbind (a, regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_DESTROY:
    {
      pair<int, lily_error_t> r = a->destroy (a, regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_UNBOUND:
    {
      pair<int, lily_error_t> r = a->subscribe_unbound (a, regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_UNBOUND:
    {
      pair<int, lily_error_t> r = a->unsubscribe_unbound (a, regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_DESTROYED:
    {
      pair<int, lily_error_t> r = a->subscribe_destroyed (a, regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_DESTROYED:
    {
      pair<int, lily_error_t> r = a->unsubscribe_destroyed (a, regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_LOG:
    {
      pair<int, lily_error_t> r = a->log (reinterpret_cast<const char*> (regs.ebx), regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_ADJUST_BREAK:
    {
      pair<void*, lily_error_t> r = a->adjust_break (regs.ebx);
      regs.eax = reinterpret_cast<uint32_t> (r.first);
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_CREATE:
    {
      pair<bd_t, lily_error_t> r = a->buffer_create (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_COPY:
    {
      pair<bd_t, lily_error_t> r = a->buffer_copy (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_DESTROY:
    {
      pair<int, lily_error_t> r = a->buffer_destroy (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_SIZE:
    {
      pair<size_t, lily_error_t> r = a->buffer_size (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_RESIZE:
    {
      pair<int, lily_error_t> r = a->buffer_resize (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_ASSIGN:
    {
      pair<int, lily_error_t> r = a->buffer_assign (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_APPEND:
    {
      pair<bd_t, lily_error_t> r = a->buffer_append (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_MAP:
    {
      pair<void*, lily_error_t> r = a->buffer_map (regs.ebx);
      regs.eax = reinterpret_cast<uint32_t> (r.first);
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_BUFFER_UNMAP:
    {
      pair<int, lily_error_t> r = a->buffer_unmap (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SYSCONF:
    {
      switch (regs.ebx) {
      case LILY_SYSCALL_SYSCONF_PAGESIZE:
	regs.eax = PAGE_SIZE;
	regs.ecx = LILY_ERROR_SUCCESS;
	return;
	break;
      default:
	regs.eax = 0;
	regs.ecx = LILY_ERROR_INVAL;
	return;
	break;
      }
      return;
    }
    break;
  case LILY_SYSCALL_DESCRIBE:
    {
      pair<int, lily_error_t> r = a->describe (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_GETAID:
    {
      regs.eax = a->aid ();
      regs.ecx = LILY_ERROR_SUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETINITA:
    {
      regs.eax = a->inita ();
      regs.ecx = LILY_ERROR_SUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETINITB:
    {
      regs.eax = a->initb ();
      regs.ecx = LILY_ERROR_SUCCESS;
      return;
    }
    break;
  case LILY_SYSCALL_GETMONOTIME:
    {
      pair<int, lily_error_t> r = a->getmonotime (reinterpret_cast<mono_time_t*> (regs.ebx));
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_MAP:
    {
      pair<int, lily_error_t> r = a->map (reinterpret_cast<const void*> (regs.ebx), reinterpret_cast<const void*> (regs.ecx), regs.edx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNMAP:
    {
      pair<int, lily_error_t> r = a->unmap (reinterpret_cast<const void*> (regs.ebx));
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_RESERVE_PORT:
    {
      pair<int, lily_error_t> r = a->reserve_port (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNRESERVE_PORT:
    {
      pair<int, lily_error_t> r = a->unreserve_port (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INB:
    {
      pair<uint8_t, lily_error_t> r = a->inb (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTB:
    {
      pair<int, lily_error_t> r = a->outb (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INW:
    {
      pair<uint16_t, lily_error_t> r = a->inw (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTW:
    {
      pair<int, lily_error_t> r = a->outw (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_INL:
    {
      pair<uint32_t, lily_error_t> r = a->inl (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_OUTL:
    {
      pair<int, lily_error_t> r = a->outl (regs.ebx, regs.ecx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_SUBSCRIBE_IRQ:
    {
      pair<int, lily_error_t> r = a->subscribe_irq (a, regs.ebx, regs.ecx, regs.edx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  case LILY_SYSCALL_UNSUBSCRIBE_IRQ:
    {
      pair<int, lily_error_t> r = a->unsubscribe_irq (regs.ebx);
      regs.eax = r.first;
      regs.ecx = r.second;
      return;
    }
    break;
  default:
    kpanic ("TODO:  Unknown system call");
    break;
  }
}
