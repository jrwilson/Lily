#include <lily/syscall.h>

	.section .data
	.global automatonerrno
automatonerrno:	
	.long 0

	.section .text

	.global finish
finish:
	mov $LILY_SYSCALL_FINISH, %eax
	int $0x80

	.global exit
exit:
	mov $LILY_SYSCALL_EXIT, %eax
	int $0x80

	.global create
create:
	mov $LILY_SYSCALL_CREATE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global bind
bind:
	mov $LILY_SYSCALL_BIND, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global subscribe_destroyed
subscribe_destroyed:
	mov $LILY_SYSCALL_SUBSCRIBE_DESTROYED, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret
	
	.global buffer_create
buffer_create:
	mov $LILY_SYSCALL_BUFFER_CREATE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_copy
buffer_copy:
	mov $LILY_SYSCALL_BUFFER_COPY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_resize
buffer_resize:
	mov $LILY_SYSCALL_BUFFER_RESIZE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_append
buffer_append:
	mov $LILY_SYSCALL_BUFFER_APPEND, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

/* inline int */
/* buffer_assign (bid_t dest, */
/* 	       size_t dest_offset, */
/* 	       bid_t src, */
/* 	       size_t src_offset, */
/* 	       size_t length) */
/* { */
/*   int result; */
/*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_ASSIGN), "b"(dest), "c"(dest_offset), "d"(src), "S"(src_offset), "D"(length) :); */
/*   return result; */
/* } */

	.global buffer_map
buffer_map:
	mov $LILY_SYSCALL_BUFFER_MAP, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_unmap
buffer_unmap:
	mov $LILY_SYSCALL_BUFFER_UNMAP, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_destroy
buffer_destroy:
	mov $LILY_SYSCALL_BUFFER_DESTROY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_capacity
buffer_capacity:
	mov $LILY_SYSCALL_BUFFER_CAPACITY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global sysconf
sysconf:
	mov $LILY_SYSCALL_SYSCONF, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global set_registry
set_registry:
	mov $LILY_SYSCALL_SET_REGISTRY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global get_registry
get_registry:
	mov $LILY_SYSCALL_GET_REGISTRY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret
