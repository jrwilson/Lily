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

namespace system {
  
  enum {
    FINISH = 0,
    GETPAGESIZE,
    SBRK,
  };

}

#endif /* __syscall_def_hpp__ */
