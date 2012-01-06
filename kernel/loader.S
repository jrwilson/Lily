	# Authors
	#   http://http://wiki.osdev.org/Higher_Half_With_GDT
	#   Justin R. Wilson

	# Starting virtual address of the kernel.  Should agree with mm.c.
	.set KERNEL_VIRTUAL_BASE, 0xC0000000
	
	# The size of the stack.
	.set STACK_SIZE, 0x1000
	# The alignment of the stack.
	.set STACK_ALIGN, 16
	
	.section .setup, "a"

	# Multiboot constants.
	# Bootloader should place things on page boundaries.
	.set MULTIBOOT_PAGE_ALIGN, 1<<0
	# Bootloader should provide a memory map.
	.set MULTIBOOT_MEMORY_INFO, 1<<1
	.set MULTIBOOT_HEADER_MAGIC, 0x1BADB002
	.set MULTIBOOT_HEADER_FLAGS, MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
	.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	# Multiboot header.
	.balign 4
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long MULTIBOOT_CHECKSUM

	.set PAGE_PRESENT, (1 << 0)
	.set PAGE_WRITABLE, (1 << 1)
	.set PAGE_USER, (1 << 2)
	.set PAGE_SUPERVISOR, (0 << 2)	
	
	.set PAGE_DIRECTORY_LOW_ENTRY, (0 >> 22)
	.set PAGE_DIRECTORY_HIGH_ENTRY, (KERNEL_VIRTUAL_BASE >> 22)
	.set PAGE_TABLE_ENTRY, ((KERNEL_VIRTUAL_BASE >> 12) & 0x3FF)

	# Paging structures.
	.balign 4096, 0
	.global kpd
kpd:
	.global kernel_page_directory
kernel_page_directory:
	.space 4096, 0
	.global kernel_page_table
kernel_page_table:
	.space 4096, 0
	# A zero frame that we will use for copy on write.
	.global zero_page
zero_page:
	.space 4096, 0
	
	# Export the start symbol so the linker can find it.
	.global start
start:
	# Disable interrupts.
	cli
	# Setup a stack.
	mov $(stack_end - KERNEL_VIRTUAL_BASE), %esp
	# Push the pointer to the multiboot info structure.
	push %ebx
	# Push the multiboot magic number.
	push %eax
	# Initialize the page directory.
	mov $kernel_page_table, %ecx
	or $(PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER), %ecx
	# Page table is mapped in both locations.
	mov %ecx, (kernel_page_directory + PAGE_DIRECTORY_LOW_ENTRY * 4)
	mov %ecx, (kernel_page_directory + PAGE_DIRECTORY_HIGH_ENTRY* 4)
	# Map page directory to itself.
	mov $kernel_page_directory, %ecx
	or $(PAGE_PRESENT | PAGE_WRITABLE | PAGE_SUPERVISOR), %ecx
	mov %ecx, (kernel_page_directory + 4 * 1023)
	# Initialize the page table.  Map the first 4MB.
	xor %eax, %eax
	mov $0x400000, %ebx
loop1:
	cmp %eax, %ebx
	jle loop2
	mov %eax, %ecx
	shr $12, %ecx
	and $0x3FF, %ecx
	mov %eax, %edx
	or $(PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER), %edx
	mov %edx, kernel_page_table(, %ecx, 4)
	add $0x1000, %eax
	jmp loop1
loop2:
	# Initialize paging.
	mov $kernel_page_directory, %eax
	mov %eax, %cr3
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0
	jmp highhalf

	.section .text
highhalf:	
	# Setup the stack in order to call kmain.
	add $KERNEL_VIRTUAL_BASE, %esp
	
	# Import the kmain symbol.
	.extern kmain
	call kmain

	.section .bss
	# Reserve space for the stack.
	# TODO:  Should the stack be page-aligned?
	.balign STACK_ALIGN
	.global stack_begin
stack_begin:	
	.space STACK_SIZE
	.global stack_end
stack_end:	
