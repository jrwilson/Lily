#include "syscall.h"
#include <lily/syscall.h>

void
finish (const void* action_entry_point,
	const void* parameter,
	const void* value,
	size_t value_size,
	bid_t buffer,
	size_t buffer_size)
{
  asm ("int $0x80\n" : : "a"(LILY_SYSCALL_FINISH));
}

