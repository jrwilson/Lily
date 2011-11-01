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
#include "interrupt.h"
#include "kassert.h"
#include "scheduler.h"

static void
syscall_handler (registers_t* regs)
{
  syscall_t syscall = regs->eax;
  switch (syscall) {
  case SYSCALL_FINISH:
    finish_action ();
    break;
  case SYSCALL_SCHEDULE:
    schedule_action (regs->ebx, regs->ecx);
    finish_action ();
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

