#include <lily/syscall.h>

	.global finish
finish:
	mov $LILY_SYSCALL_FINISH, %eax
	int $0x80
	