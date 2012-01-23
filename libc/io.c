#include "io.h"
#include <lily/syscall.h>
#include "syscall_errno.h"

int
map (const void* destination,
     const void* source,
     size_t size)
{
  asm ("int $0x80\n"
       "mov %%ebx, %0\n": "=m"(syscall_errno) : "a"(LILY_SYSCALL_MAP));
}

