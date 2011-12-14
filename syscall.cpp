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
#include "kassert.hpp"

namespace system {
  
  void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  bool output_status,
	  const void* output_buffer)
  {
    asm ("int $0x80\n" : : "a"(FINISH), "b"(action_entry_point), "c"(parameter), "d"(output_status), "S"(output_buffer) :);
  }
  
  size_t
  getpagesize (void)
  {
    size_t retval;
    asm ("int $0x80\n" : "=b"(retval) : "a"(GETPAGESIZE) :);
    return retval;
}
  
  void*
  sbrk (ptrdiff_t size)
  {
    void* ptr;
    asm ("int $0x80\n" : "=b"(ptr) : "a"(SBRK), "b"(size) :);
    return ptr;
  }
  
}
