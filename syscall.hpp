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

namespace system {
  
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

  bid_t
  buffer_create (size_t size = 0);

  bid_t
  buffer_create (bid_t bid,
		 size_t offset = 0,
		 size_t length = 0);

  size_t
  buffer_append (bid_t bid,
		 size_t size);

  size_t
  buffer_append (bid_t dest,
		 bid_t src,
		 size_t offset = 0,
		 size_t length = 0);

  void*
  buffer_map (bid_t bid);

  int
  buffer_destroy (bid_t bid);

  size_t
  buffer_size (bid_t bid);
}

#endif /* __syscall_hpp__ */
