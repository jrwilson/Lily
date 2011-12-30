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

namespace syscall {
  
  enum {
    FINISH = 0,
    GETPAGESIZE,
    SBRK,
    BINDING_COUNT,
    BUFFER_CREATE,
    BUFFER_COPY,
    BUFFER_GROW,
    BUFFER_APPEND,
    BUFFER_ASSIGN,
    BUFFER_MAP,
    BUFFER_DESTROY,
    BUFFER_SIZE,
  };

}

#endif /* __syscall_def_hpp__ */
