	# Authors
	#   Justin R. Wilson

	.include "selectors.S"

.macro TRAP trap_num int_num
	.global trap\trap_num
trap\trap_num:
	push $0
	push $\int_num
	jmp trap_common_stub
.endm

	# Import trap_dispatch.
	.extern trap_dispatch
trap_common_stub:
	# Push the processor state.
	pusha
	# TODO:  Is all this segment manipulation necessary?
	# Push the old data segment.
	mov %ds, %ax
	push %eax
	# Load the kernel data segment.
	mov $KERNEL_DATA_SELECTOR, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	call trap_dispatch
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

TRAP 0 0x80
TRAP 1 0x81
TRAP 2 0x82
	