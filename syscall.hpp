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

#include "types.hpp"

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

}

#endif /* __syscall_hpp__ */
