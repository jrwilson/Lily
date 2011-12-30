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

namespace syscall {
  
  void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  bool status,
	  bid_t bid,
	  const void* buffer);
  
  size_t
  getpagesize (void);
  
  void*
  sbrk (ptrdiff_t);

  // TODO:  Use action traits to enforce parameter vs. no parameter.
  size_t
  binding_count (const void*,
		 aid_t parameter);

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

  bid_t
  buffer_create (size_t size);

  bid_t
  buffer_copy (bid_t bid,
	       size_t offset,
	       size_t length);
  
  size_t
  buffer_grow (bid_t bid,
	       size_t size);
  
  size_t
  buffer_append (bid_t dest,
		 bid_t src,
		 size_t offset,
		 size_t length);

  int
  buffer_assign (bid_t dest,
		 size_t dest_offset,
		 bid_t src,
		 size_t src_offset,
		 size_t length);
  
  // TODO:  Use action traits to map the buffer with the correct type.
  void*
  buffer_map (bid_t bid);

  int
  buffer_destroy (bid_t bid);

  size_t
  buffer_size (bid_t bid);
}

#endif /* __syscall_hpp__ */
