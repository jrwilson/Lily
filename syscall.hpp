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

#include "syscall_def.hpp"
#include "types.hpp"

// action_entry_point == 0 means do not schedule an action.
// buffer == 0 means no output was produced.
void
sys_finish (size_t action_entry_point,
	    void* parameter,
	    void* output_buffer);

size_t
sys_get_page_size (void);

void*
sys_allocate (size_t);

#endif /* __syscall_hpp__ */
