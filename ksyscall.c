/*
  File
  ----
  syscall.c
  
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

static void
syscall_handler (registers_t* regs)
{
  syscall_t syscall = EXTRACT_SYSCALL (regs->eax);
  unsigned int action_entry_point = regs->ebx;
  unsigned int parameter = regs->ecx;
  int output_status = IS_OUTPUT_VALID (regs->eax);
  unsigned int output_value = regs->edx;

  switch (syscall) {
  case SYSCALL_FINISH:
    finish_action (output_status, output_value);
    break;
  case SYSCALL_SCHEDULE:
    {
      schedule_action (get_current_aid (), action_entry_point, parameter);
      finish_action (output_status, output_value);
    }
    break;
  }

  /* TODO:  Unknown syscall. */
  kassert (0);
}

void
initialize_syscalls ()
{
  set_interrupt_handler (TRAP_BASE + 0, syscall_handler);
}

