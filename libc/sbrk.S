#include <lily/syscall.h>

	.section .text
	.global adjust_break
adjust_break:
	mov $LILY_SYSCALL_ADJUST_BREAK, %eax
	int $0x80
	ret
	
