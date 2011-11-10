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

#define OUTPUT_VALID_MASK (1 << 31)
#define IS_OUTPUT_VALID(x) ((x) & OUTPUT_VALID_MASK)
#define EXTRACT_SYSCALL(x) ((x) & 0x7FFFFFFF)

typedef enum {
  SYSCALL_FINISH = 0,
  SYSCALL_SCHEDULE,
  SYSCALL_GET_PAGE_SIZE,
  SYSCALL_ALLOCATE,
} syscall_t;

typedef enum {
  SYSERROR_SUCCESS = 0,
  SYSERROR_REQUESTED_SIZE_NOT_ALIGNED,
  SYSERROR_OUT_OF_MEMORY,
} syserror_t;

#endif /* __syscall_defh__ */
