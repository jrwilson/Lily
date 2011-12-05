#ifndef __syscall_def_hpp__
#define __syscall_def_hpp__

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

#include "types.hpp"

enum syscall_t {
  SYSCALL_FINISH = 0,
  SYSCALL_GET_PAGE_SIZE,
  SYSCALL_ALLOCATE,
};

enum syserror_t {
  SYSERROR_SUCCESS = 0,
  SYSERROR_OUT_OF_MEMORY,
};

#endif /* __syscall_def_hpp__ */
