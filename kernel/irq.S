	/* Authors
	   Justin R. Wilson
	*/

	.include "selectors.S"

.macro IRQ irq_num int_num
	.global irq\irq_num
irq\irq_num:
	push $0		# Error code.
	push $\int_num	# Interrupt number.
	jmp irq_common_stub
.endm

	# Import irq_dispatch.
	.extern irq_dispatch
irq_common_stub:
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
	call irq_dispatch
	# Pop the old data segment.
	pop %eax
	# Load the old data segment.
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	# Pop the processor state.
	popa
	# Pop the error and interrupt number.
	add $8, %esp
	iret
	
IRQ 0 32
IRQ 1 33
IRQ 2 34
IRQ 3 35
IRQ 4 36
IRQ 5 37
IRQ 6 38
IRQ 7 39
IRQ 8 40
IRQ 9 41
IRQ 10 42
IRQ 11 43
IRQ 12 44
IRQ 13 45
IRQ 14 46
IRQ 15 47
