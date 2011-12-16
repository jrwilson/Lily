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
  
  size_t
  buffer_size (bid_t bid)
  {
    size_t size;
    asm ("int $0x80\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :);
    return size;
  }

  int
  buffer_incref (bid_t bid)
  {
    int refcount;
    asm ("int $0x80\n" : "=a"(refcount) : "0"(BUFFER_INCREF), "b"(bid) :);
    return refcount;
  }

  int
  buffer_decref (bid_t bid)
  {
    int refcount;
    asm ("int $0x80\n" : "=a"(refcount) : "0"(BUFFER_DECREF), "b"(bid) :);
    return refcount;
  }

  int
  buffer_addchild (bid_t parent,
		   bid_t child)
  {
    int result;
    asm ("int $0x80\n" : "=a"(result) : "0"(BUFFER_ADDCHILD), "b"(parent), "c"(child) :);
    return result;
  }

}
