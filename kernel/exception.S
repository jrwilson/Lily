	/* Authors
	   Justin R. Wilson
	*/

	.include "selectors.S"

.macro EXCEPTION_NOERRCODE num
	.global exception\num
exception\num:
	push $0
	push $\num
	jmp exception_common_stub
.endm

.macro EXCEPTION_ERRCODE num
	.global exception\num
exception\num:
	push $\num
	jmp exception_common_stub
.endm

	# Import exception_handler.
	.extern exception_dispatch
exception_common_stub:
	# Push the processor state.
	pusha
	# Push the old data segment.
	mov %ds, %ax
	push %eax
	# Load the kernel data segment.
	mov $KERNEL_DATA_SELECTOR, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	# TODO:  What about the stack segment?
	call exception_dispatch
	# Pop the old data segment.
	pop %eax
	# Load the old data segment.
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	# Pop the processor state.
	popa
	# Pop the interrupt number and error code.
	add $8, %esp
	iret
	
EXCEPTION_NOERRCODE 0
EXCEPTION_NOERRCODE 1
EXCEPTION_NOERRCODE 2
EXCEPTION_NOERRCODE 3
EXCEPTION_NOERRCODE 4
EXCEPTION_NOERRCODE 5
EXCEPTION_NOERRCODE 6
EXCEPTION_NOERRCODE 7
EXCEPTION_ERRCODE 8
EXCEPTION_NOERRCODE 9
EXCEPTION_ERRCODE 10
EXCEPTION_ERRCODE 11
EXCEPTION_ERRCODE 12
EXCEPTION_ERRCODE 13
EXCEPTION_ERRCODE 14
EXCEPTION_NOERRCODE 15
EXCEPTION_NOERRCODE 16
EXCEPTION_NOERRCODE 17
EXCEPTION_NOERRCODE 18
EXCEPTION_NOERRCODE 19
EXCEPTION_NOERRCODE 20
EXCEPTION_NOERRCODE 21
EXCEPTION_NOERRCODE 22
EXCEPTION_NOERRCODE 23
EXCEPTION_NOERRCODE 24
EXCEPTION_NOERRCODE 25
EXCEPTION_NOERRCODE 26
EXCEPTION_NOERRCODE 27
EXCEPTION_NOERRCODE 28
EXCEPTION_NOERRCODE 29
EXCEPTION_NOERRCODE 30
EXCEPTION_NOERRCODE 31
