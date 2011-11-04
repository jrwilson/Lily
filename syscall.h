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

#include "syscall_def.h"

void
sys_finish (int output_status,
	    unsigned int output_value);

void
sys_schedule (unsigned int action_entry_point,
	      unsigned int parameter,
	      int output_status,
	      unsigned int output_value);

unsigned int
sys_get_page_size (void);

void*
sys_allocate (unsigned int size);

#endif /* __syscall_h__ */
