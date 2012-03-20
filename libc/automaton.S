#include <lily/syscall.h>

	.section .data
	.global automatonerrno
automatonerrno:	
	.long 0

	.section .text

	.global schedule
schedule:
	mov $LILY_SYSCALL_SCHEDULE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

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

	.global unbind
unbind:
	mov $LILY_SYSCALL_UNBIND, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global destroy
destroy:
	mov $LILY_SYSCALL_DESTROY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global subscribe_destroyed
subscribe_destroyed:
	mov $LILY_SYSCALL_SUBSCRIBE_DESTROYED, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global adjust_break
adjust_break:
	mov $LILY_SYSCALL_ADJUST_BREAK, %eax
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

	.global buffer_destroy
buffer_destroy:
	mov $LILY_SYSCALL_BUFFER_DESTROY, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_size
buffer_size:
	mov $LILY_SYSCALL_BUFFER_SIZE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret
	
	.global buffer_resize
buffer_resize:
	mov $LILY_SYSCALL_BUFFER_RESIZE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_assign
buffer_assign:
	mov $LILY_SYSCALL_BUFFER_ASSIGN, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global buffer_append
buffer_append:
	mov $LILY_SYSCALL_BUFFER_APPEND, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

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

	.global sysconf
sysconf:
	mov $LILY_SYSCALL_SYSCONF, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global lookup
lookup:
	mov $LILY_SYSCALL_LOOKUP, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global describe
describe:
	mov $LILY_SYSCALL_DESCRIBE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global getaid
getaid:
	mov $LILY_SYSCALL_GETAID, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global getinita
getinita:
	mov $LILY_SYSCALL_GETINITA, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global getinitb
getinitb:
	mov $LILY_SYSCALL_GETINITB, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

	.global getmonotime
getmonotime:
	mov $LILY_SYSCALL_GETMONOTIME, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret
