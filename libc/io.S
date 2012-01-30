#include <lily/syscall.h>

	.section .data
	.global ioerrno
ioerrno:	
	.long 0

	.section .text
	.global map
map:
	mov $LILY_SYSCALL_MAP, %eax
	int $0x80
	mov %ecx, ioerrno
	ret
	
	.global reserve_port
reserve_port:
	mov $LILY_SYSCALL_RESERVE_PORT, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global inb
inb:
	mov $LILY_SYSCALL_INB, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global outb
outb:
	mov $LILY_SYSCALL_OUTB, %eax
	int $0x80
	mov %ecx, ioerrno
	ret
