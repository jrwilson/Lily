/*
  File
  ----
  syscall.c
  
  Description
  -----------
  System calls for users.

  Authors:
  Justin R. Wilson
*/

#include "syscall.h"

void
sys_finish ()
{
  asm volatile ("mov %0, %%eax\n"
		"int $0x80\n" : : "r"(SYSCALL_FINISH));
}

void
sys_schedule (unsigned int action_entry_point,
	      unsigned int parameter)
{
  asm volatile ("mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"int $0x80\n" : : "r"(SYSCALL_SCHEDULE), "m"(action_entry_point), "m"(parameter));
}
