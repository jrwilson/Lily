	/*
	   Authors
	   http://http://wiki.osdev.org/Higher_Half_With_GDT
	   Justin R. Wilson
	*/

	.include "selectors.S"
	
	.global gdt_flush
gdt_flush:
	mov 4(%esp), %eax
	# Load the GDT.
	lgdt (%eax)
	# Change all of the data segments.
	mov $KERNEL_DATA_SELECTOR, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	# Change the code segment by executing a far jump.
	ljmp $KERNEL_CODE_SELECTOR, $gdt_flush2

gdt_flush2:
	ret
