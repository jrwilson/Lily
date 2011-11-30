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

static const uint32_t SCHEDULE_VALID_MASK = (1 << 31);
static const uint32_t OUTPUT_VALID_MASK = (1 << 30);

inline bool
is_schedule_valid (uint32_t code)
{
  return (code & SCHEDULE_VALID_MASK) != 0;
}

inline bool
is_output_valid (uint32_t code)
{
  return (code & OUTPUT_VALID_MASK) != 0;
}

inline syscall_t
extract_syscall (uint32_t code)
{
  return static_cast<syscall_t> (code & 0x3FFFFFFF);
}

#endif /* __syscall_def_hpp__ */
