	/* Authors
	   Justin R. Wilson
	*/

	.include "selectors.S"

	.section .data

	/* Bitset of IRQs that have fired. */
	.global irq_set
irq_set:
	.long 0

	.section .text

	.set PIC_MASTER_LOW,  0x20
	.set PIC_SLAVE_LOW, 0xA0

	.set PIC_OCW2_LOW, 0
	.set PIC_OCW2_NON_SPECIFIC_EOI, 1<<5

.macro MASTER_IRQ irq_num
	.global irq\irq_num
irq\irq_num:
	/* Set the bit. */
	orl $(1 << \irq_num), irq_set
	/* Send end of interrupt. */
	push %eax
	mov $(PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI), %al
	outb $PIC_MASTER_LOW
	pop %eax
	iret
.endm

.macro SLAVE_IRQ irq_num
	.global irq\irq_num
irq\irq_num:
	/* Set the bit. */
	orl $(1 << \irq_num), irq_set
	/* Send end of interrupt. */
	push %eax
	mov $(PIC_OCW2_LOW | PIC_OCW2_NON_SPECIFIC_EOI), %al
	outb $PIC_SLAVE_LOW
	outb $PIC_MASTER_LOW
	pop %eax
	iret
.endm

MASTER_IRQ 0
MASTER_IRQ 1
MASTER_IRQ 2
MASTER_IRQ 3
MASTER_IRQ 4
MASTER_IRQ 5
MASTER_IRQ 6
MASTER_IRQ 7
SLAVE_IRQ 8
SLAVE_IRQ 9
SLAVE_IRQ 10
SLAVE_IRQ 11
SLAVE_IRQ 12
SLAVE_IRQ 13
SLAVE_IRQ 14
SLAVE_IRQ 15

/*
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
*/

/*
MASTER_IRQ 0 32
MASTER_IRQ 1 33
MASTER_IRQ 2 34
MASTER_IRQ 3 35
MASTER_IRQ 4 36
MASTER_IRQ 5 37
MASTER_IRQ 6 38
MASTER_IRQ 7 39
SLAVE_IRQ 8 40
SLAVE_IRQ 9 41
SLAVE_IRQ 10 42
SLAVE_IRQ 11 43
SLAVE_IRQ 12 44
SLAVE_IRQ 13 45
SLAVE_IRQ 14 46
SLAVE_IRQ 15 47
*/
