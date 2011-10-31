#ifndef __syscall_h__
#define __syscall_h__

/*
  File
  ----
  syscall.h
  
  Description
  -----------
  Declarations for functions to manage system calls.

  Authors:
  Justin R. Wilson
*/

typedef enum {
  SYSCALL_FINISH = 0,
  SYSCALL_SCHEDULE = 1,
} syscall_t;

void
initialize_syscalls (void);

#endif /* __syscall_h__ */
