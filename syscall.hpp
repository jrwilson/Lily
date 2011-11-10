#ifndef __syscall_h__
#define __syscall_h__

/*
  File
  ----
  syscall.h
  
  Description
  -----------
  System calls for users.

  Authors:
  Justin R. Wilson
*/

#include "syscall_def.hpp"
#include "types.hpp"

void
sys_finish (int output_status,
	    value_t output_value);

void
sys_schedule (void* action_entry_point,
	      parameter_t parameter,
	      int output_status,
	      value_t output_value);

size_t
sys_get_page_size (void);

void*
sys_allocate (size_t);

#endif /* __syscall_h__ */
