#ifndef __syscall_def_h__
#define __syscall_def_h__

/*
  File
  ----
  syscall_def.h
  
  Description
  -----------
  Definitions for system calls.

  Authors:
  Justin R. Wilson
*/

typedef enum {
  SYSCALL_FINISH = 0,
  SYSCALL_SCHEDULE = 1,
} syscall_t;

#endif /* __syscall_defh__ */
