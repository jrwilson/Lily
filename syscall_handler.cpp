/*
  File
  ----
  syscall_handler.c
  
  Description
  -----------
  Functions for system calls.

  Authors
  -------
  Justin R. Wilson
*/

#include "syscall.hpp"
#include "idt.hpp"
#include "kassert.hpp"
#include "scheduler.hpp"
#include "system_automaton.hpp"
#include "vm_manager.hpp"

static void
syscall_handler (registers_t* regs)
{
  syscall_t syscall = static_cast<syscall_t> (EXTRACT_SYSCALL (regs->eax));

  switch (syscall) {
  case SYSCALL_FINISH:
    {
      int output_status = IS_OUTPUT_VALID (regs->eax);
      value_t output_value = regs->edx;
      finish_action (output_status, output_value);
      return;
    }
    break;
  case SYSCALL_SCHEDULE:
    {
      void* action_entry_point = (void*)regs->ebx;
      parameter_t parameter = regs->ecx;
      int output_status = IS_OUTPUT_VALID (regs->eax);
      value_t output_value = regs->edx;
      schedule_action (scheduler_get_current_automaton (), action_entry_point, parameter);
      finish_action (output_status, output_value);
      return;
    }
    break;
  case SYSCALL_GET_PAGE_SIZE:
    {
      regs->eax = 0;
      regs->ebx = PAGE_SIZE;
      return;
    }
    break;
  case SYSCALL_ALLOCATE:
    {
      size_t size = regs->ebx;
      syserror_t error = SYSERROR_SUCCESS;
      logical_address ptr = scheduler_get_current_automaton ()->alloc (size, &error);
      regs->eax = error;
      regs->ebx = reinterpret_cast<uint32_t> (ptr.value ());
      return;
    }
    break;
  }

  /* TODO:  Unknown syscall. */  
  kputs ("Unknown syscall\n");
  halt ();
}

void
syscall_handler_initialize ()
{
  set_interrupt_handler (TRAP_BASE + 0, syscall_handler);
}

