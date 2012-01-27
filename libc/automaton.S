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

	.global binding_count
binding_count:
	mov $LILY_SYSCALL_BINDING_COUNT, %eax
	mov %ecx, automatonerrno
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

	.global buffer_create
buffer_create:
	mov $LILY_SYSCALL_BUFFER_CREATE, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret

/* inline bid_t */
/* buffer_copy (bid_t b, */
/* 	     size_t offset, */
/* 	     size_t length) */
/* { */
/*   bid_t bid; */
/*   asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_COPY), "b"(b), "c"(offset), "d"(length) :); */
/*   return bid; */
/* } */

/* inline size_t */
/* buffer_grow (bid_t bid, */
/* 	     size_t size) */
/* { */
/*   size_t off; */
/*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bid), "c"(size) :); */
/*   return off; */
/* } */

/* inline size_t */
/* buffer_append (bid_t dest, */
/* 	       bid_t src, */
/* 	       size_t offset, */
/* 	       size_t length) */
/* { */
/*   size_t off; */
/*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_APPEND), "b"(dest), "c"(src), "d"(offset), "S"(length) :); */
/*   return off; */
/* } */

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

/* inline size_t */
/* buffer_size (bid_t bid) */
/* { */
/*   size_t size; */
/*   asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :); */
/*   return size; */
/* } */

	.global sysconf
sysconf:
	mov $LILY_SYSCALL_SYSCONF, %eax
	int $0x80
	mov %ecx, automatonerrno
	ret
	