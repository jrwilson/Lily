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
  
  // TODO:  Eliminate this by passing the page size during initialization.
  size_t
  getpagesize (void)
  {
    size_t size;
    asm ("int $0x80\n" : "=a"(size) : "0"(GETPAGESIZE) :);
    return size;
}
  
  void*
  sbrk (ptrdiff_t size)
  {
    void* ptr;
    asm ("int $0x80\n" : "=a"(ptr) : "0"(SBRK), "b"(size) :);
    return ptr;
  }

  bid_t
  buffer_create (size_t size)
  {
    bid_t bid;
    asm ("int $0x80\n" : "=a"(bid) : "0"(BUFFER_CREATE), "b"(size) :);
    return bid;
  }

  int
  buffer_append (bid_t dest,
		 bid_t src)
  {
    int result;
    asm ("int $0x80\n" : "=a"(result) : "0"(BUFFER_APPEND), "b"(dest), "c"(src) :);
    return result;
  }

  void*
  buffer_map (bid_t bid)
  {
    void* ptr;
    asm ("int $0x80\n" : "=a"(ptr) : "0"(BUFFER_MAP), "b"(bid) :);
    return ptr;
  }

  int
  buffer_unmap (bid_t bid)
  {
    int result;
    asm ("int $0x80\n" : "=a"(result) : "0"(BUFFER_UNMAP), "b"(bid) :);
    return result;
  }

  size_t
  buffer_size (bid_t bid)
  {
    size_t size;
    asm ("int $0x80\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :);
    return size;
  }

}
