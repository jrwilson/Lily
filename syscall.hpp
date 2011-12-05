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

void
sys_finish (bool schedule_status,
	    local_func action_entry_point,
	    void* parameter,
	    bool output_status,
	    void* output_buffer);

size_t
sys_get_page_size (void);

void*
sys_allocate (size_t);

#endif /* __syscall_hpp__ */
