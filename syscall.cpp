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

#include "syscall.hpp"
#include "kput.hpp"

static syserror_t errno;

void
sys_finish (bool output_status,
	    value_t output_value)
{
  asm volatile ("mov %0, %%eax\n"
		"mov %1, %%edx\n"
		"int $0x80\n" : : "r"(output_status ? SYSCALL_FINISH | OUTPUT_VALID_MASK : SYSCALL_FINISH), "m"(output_value) : "eax", "edx");
}

void
sys_schedule (void* action_entry_point,
	      parameter_t parameter,
	      bool output_status,
	      value_t output_value)
{
  asm volatile ("mov %0, %%eax\n"
		"mov %1, %%ebx\n"
		"mov %2, %%ecx\n"
		"mov %3, %%edx\n"
		"int $0x80\n" : : "r"(output_status ? SYSCALL_SCHEDULE | OUTPUT_VALID_MASK : SYSCALL_SCHEDULE), "m"(action_entry_point), "m"(parameter), "m"(output_value) : "eax", "ebx", "ecx", "edx");
}

size_t
sys_get_page_size (void)
{
  size_t retval;
  asm volatile ("mov %2, %%eax\n"
		"int $0x80\n"
		"mov %%eax, %0\n"
		"mov %%ebx, %1\n" : "=m"(errno), "=m"(retval) : "r"(SYSCALL_GET_PAGE_SIZE) : "eax", "ebx");

  return retval;
}

void*
sys_allocate (size_t size)
{
  void* ptr;
  asm volatile ("mov %2, %%eax\n"
		"mov %3, %%ebx\n"
		"int $0x80\n"
		"mov %%eax, %0\n"
		"mov %%ebx, %1\n" : "=m"(errno), "=m"(ptr) : "r"(SYSCALL_ALLOCATE), "m"(size) : "eax", "ebx");

  return ptr;
}
