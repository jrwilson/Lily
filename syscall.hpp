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
  
  // action_entry_point == 0 means do not schedule an action.
  // buffer == 0 means no output was produced.
  void
  finish (const void* action_entry_point,
	  aid_t parameter,
	  bool output_status,
	  const void* output_buffer);
  
  size_t
  getpagesize (void);
  
  void*
  sbrk (ptrdiff_t);

  bid_t
  buffer_create (size_t size);

  int
  buffer_append (bid_t dest,
		 bid_t src);

  void*
  buffer_map (bid_t bid);

  int
  buffer_unmap (bid_t bid);

  size_t
  buffer_size (bid_t bid);
}

#endif /* __syscall_hpp__ */
