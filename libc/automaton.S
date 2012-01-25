#include <lily/syscall.h>

	.section .data
	.global automatonerrno
automatonerrno:	
	.long 0

	.section .text

	.global create
create:
	mov $LILY_SYSCALL_CREATE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

