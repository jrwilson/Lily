#include <lily/syscall.h>

	.section .data
	.global sysconferrno
sysconferrno:
	.long 0

	.section .text
	.global sysconf
sysconf:
	mov $LILY_SYSCALL_SYSCONF, %eax
	int $0x80
	mov %ecx, sysconferrno
	ret
	
