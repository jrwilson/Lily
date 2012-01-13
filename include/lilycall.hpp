#ifndef __syscall_hpp__
#define __syscall_hpp__

/*
  File
  ----
  syscall.hpp
  
  Description
  -----------
  System calls for users.

  Authors:
  Justin R. Wilson
*/

#include "aid.hpp"
#include "bid.hpp"
#include <stddef.h>

namespace lilycall {

  enum {
    CREATE = 0,
    BIND = 1,
    LOOSE = 2,
    DESTROY = 3,
    
    SBRK = 4,

    BINDING_COUNT = 5,

    BUFFER_CREATE = 6,
    BUFFER_COPY = 7,
    BUFFER_GROW = 8,
    BUFFER_APPEND = 9,
    BUFFER_ASSIGN = 10,
    BUFFER_MAP = 11,
    BUFFER_UNMAP = 12,
    BUFFER_DESTROY = 13,
    BUFFER_SIZE = 14,
  };

  inline void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  const void* copy_value,
	  size_t copy_value_size,
	  bid_t buffer,
	  size_t buffer_size)
  {
    asm ("int $0x80\n" : : "a"(action_entry_point), "b"(parameter), "c"(copy_value), "d"(copy_value_size), "S"(buffer), "D"(buffer_size) :);
  }

  inline aid_t
  create (bid_t automaton_bid,
	  size_t automaton_size)
  {
    aid_t aid;
    asm ("int $0x81\n" : "=a"(aid) : "a"(CREATE), "b"(automaton_bid), "c"(automaton_size) :);
    return aid;
  }

  inline void
  bind (void)
  {
    asm ("int $0x81\n" : : "a"(BIND) :);
  }

  inline void
  loose (void)
  {
    asm ("int $0x81\n" : : "a"(LOOSE) :);
  }

  inline void
  destroy (void)
  {
    asm ("int $0x81\n" : : "a"(DESTROY) :);
  }
  
  inline void*
  sbrk (ptrdiff_t size)
  {
    void* ptr;
    asm ("int $0x81\n" : "=a"(ptr) : "0"(SBRK), "b"(size) :);
    return ptr;
  }

  // TODO:  Use action traits to enforce parameter vs. no parameter.
  inline size_t
  binding_count (const void* ptr,
		 aid_t parameter)
  {
    size_t count;
    asm ("int $0x81\n" : "=a"(count) : "0"(BINDING_COUNT), "b"(ptr), "c"(parameter) :);
    return count;
  }

  inline size_t
  binding_count (void (*ptr) (void))
  {
    return binding_count (reinterpret_cast<const void*> (ptr), 0);
  }

  template <typename T1>
  inline size_t
  binding_count (void (*ptr) (T1))
  {
    return binding_count (reinterpret_cast<const void*> (ptr), 0);
  }

  template <typename P>
  inline size_t
  binding_count (void (*ptr) (P),
		 P parameter)
  {
    return binding_count (reinterpret_cast<const void*> (ptr), aid_cast (parameter));
  }

  template <typename T1, typename T2>
  inline size_t
  binding_count (void (*ptr) (T1, T2))
  {
    return binding_count (reinterpret_cast<const void*> (ptr), 0);
  }

  template <typename P, typename T2>
  inline size_t
  binding_count (void (*ptr) (P, T2),
		 P parameter)
  {
    return binding_count (reinterpret_cast<const void*> (ptr), aid_cast (parameter));
  }

  template <typename T1, typename T2, typename T3>
  inline size_t
  binding_count (void (*ptr) (T1, T2, T3),
		 T1 parameter)
  {
    return binding_count (reinterpret_cast<const void*> (ptr), aid_cast (parameter));
  }

  inline bid_t
  buffer_create (size_t size)
  {
    bid_t bid;
    asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_CREATE), "b"(size) :);
    return bid;
  }

  inline bid_t
  buffer_copy (bid_t b,
	       size_t offset,
	       size_t length)
  {
    bid_t bid;
    asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_COPY), "b"(b), "c"(offset), "d"(length) :);
    return bid;
  }

  inline size_t
  buffer_grow (bid_t bid,
	       size_t size)
  {
    size_t off;
    asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bid), "c"(size) :);
    return off;
  }

  inline size_t
  buffer_append (bid_t dest,
		 bid_t src,
		 size_t offset,
		 size_t length)
  {
    size_t off;
    asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_APPEND), "b"(dest), "c"(src), "d"(offset), "S"(length) :);
    return off;
  }

  inline int
  buffer_assign (bid_t dest,
		 size_t dest_offset,
		 bid_t src,
		 size_t src_offset,
		 size_t length)
  {
    int result;
    asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_ASSIGN), "b"(dest), "c"(dest_offset), "d"(src), "S"(src_offset), "D"(length) :);
    return result;
  }

  // TODO:  Use action traits to map the buffer with the correct type.
  inline void*
  buffer_map (bid_t bid)
  {
    void* ptr;
    asm ("int $0x81\n" : "=a"(ptr) : "0"(BUFFER_MAP), "b"(bid) :);
    return ptr;
  }

  inline int
  buffer_unmap (bid_t bid)
  {
    int result;
    asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_UNMAP), "b"(bid) :);
    return result;
  }

  inline int
  buffer_destroy (bid_t bid)
  {
    int result;
    asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_DESTROY), "b"(bid) :);
    return result;
  }

  inline size_t
  buffer_size (bid_t bid)
  {
    size_t size;
    asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :);
    return size;
  }

}

#endif /* __syscall_hpp__ */
