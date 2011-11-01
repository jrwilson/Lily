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
sys_finish ();

void
sys_schedule (unsigned int action_entry_point,
	      unsigned int parameter);

#endif /* __syscall_h__ */
