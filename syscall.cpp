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
#include "syscall_def.hpp"

namespace system {
  
  void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  bool output_status,
	  const void* output_buffer)
  {
    asm volatile ("mov %0, %%eax\n"
		  "mov %1, %%ebx\n"
		  "mov %2, %%ecx\n"
		  "mov %3, %%edx\n"
		  "mov %4, %%esi\n"
		  "int $0x80\n" : : "r"(FINISH), "m"(action_entry_point), "m"(parameter), "m"(output_status), "m"(output_buffer) : "eax", "ebx", "ecx", "edx", "esi");
  }
  
  size_t
  getpagesize (void)
  {
    size_t retval;
    asm volatile ("mov %1, %%eax\n"
		  "int $0x80\n"
		  "mov %%ebx, %0\n" : "=m"(retval) : "r"(GETPAGESIZE) : "eax", "ebx");
    
    return retval;
}
  
  void*
  sbrk (ptrdiff_t size)
  {
    void* ptr;
    asm volatile ("mov %1, %%eax\n"
		  "mov %2, %%ebx\n"
		  "int $0x80\n"
		  "mov %%ebx, %0\n" : "=m"(ptr) : "r"(SBRK), "m"(size) : "eax", "ebx");
    
    return ptr;
  }
  
}
