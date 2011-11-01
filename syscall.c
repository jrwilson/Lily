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
sys_finish (int output_status,
	    unsigned int output_value)
{
  asm volatile ("mov %0, %%eax\n"
		"mov %1, %%edx\n"
		"int $0x80\n" : : "r"(output_status ? SYSCALL_FINISH | OUTPUT_VALID_MASK : SYSCALL_FINISH), "m"(output_value));
}

void
sys_schedule (unsigned int action_entry_point,
	      unsigned int parameter,
	      int output_status,
	      unsigned int output_value)
{
  asm volatile ("mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n" : : "r"(output_status ? SYSCALL_SCHEDULE | OUTPUT_VALID_MASK : SYSCALL_SCHEDULE), "m"(action_entry_point), "m"(parameter), "m"(output_value));
}
