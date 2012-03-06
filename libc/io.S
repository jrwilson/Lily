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

	.global inw
inw:
	mov $LILY_SYSCALL_INW, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global outw
outw:
	mov $LILY_SYSCALL_OUTW, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global inl
inl:
	mov $LILY_SYSCALL_INL, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global outl
outl:
	mov $LILY_SYSCALL_OUTL, %eax
	int $0x80
	mov %ecx, ioerrno
	ret

	.global subscribe_irq
subscribe_irq:
	mov $LILY_SYSCALL_SUBSCRIBE_IRQ, %eax
	int $0x80
	mov %ecx, ioerrno
	ret
