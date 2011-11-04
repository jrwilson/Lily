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
      unsigned int output_value = regs->edx;
      kassert (0);
      //finish_action (output_status, output_value);
      return;
    }
    break;
  case SYSCALL_SCHEDULE:
    {
      unsigned int action_entry_point = regs->ebx;
      unsigned int parameter = regs->ecx;
      int output_status = IS_OUTPUT_VALID (regs->eax);
      unsigned int output_value = regs->edx;
      kassert (0);
      /* schedule_action (get_current_aid (), action_entry_point, parameter); */
      /* finish_action (output_status, output_value); */
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
      int size = regs->ebx;
      syserror_t error = SYSERROR_SUCCESS;
      void* ptr = automaton_allocate (scheduler_get_current_automaton (), size, &error);
      regs->eax = error;
      regs->ebx = (unsigned int)ptr;
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

