#include "io.h"
#include <lily/syscall.h>

int
map (const void* destination,
     const void* source,
     size_t size)
{
  asm ("int $0x80\n" : : "a"(LILY_SYSCALL_MAP));
}

