#include <lily/syscall.h>

	.section .text
	.global sbrk
sbrk:
	mov $LILY_SYSCALL_SBRK, %eax
	int $0x80
	ret
	
