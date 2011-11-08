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

#include "syscall.h"
#include "idt.h"
#include "kassert.h"
#include "scheduler.h"
#include "system_automaton.h"
#include "vm_manager.h"

static void
syscall_handler (registers_t* regs)
{
  syscall_t syscall = EXTRACT_SYSCALL (regs->eax);

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
      void* ptr = automaton_alloc (scheduler_get_current_automaton (), size, &error);
      regs->eax = error;
      regs->ebx = (uint32_t)ptr;
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

