	;; Authors
	;;   http://http://wiki.osdev.org/Higher_Half_With_GDT
	;;   Justin R. Wilson

	[bits 32]

	;; Starting virtual address of the kernel.  Should agree with mm.c.
	KERNEL_VIRTUAL_BASE equ 0xC0000000
	
	;; The size of the stack.
	STACK_SIZE equ 0x1000
	;; The alignment of the stack.
	STACK_ALIGN equ 16
	
	[section .setup]

	;; Multiboot constants.
	;; Bootloader should place things on page boundaries.
	MULTIBOOT_PAGE_ALIGN equ 1<<0
	;; Bootloader should provide a memory map.
	MULTIBOOT_MEMORY_INFO equ 1<<1
	MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
	MULTIBOOT_CHECKSUM equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	;; Multiboot header.
	align 4
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

	PAGE_PRESENT equ (1 << 0)
	PAGE_WRITABLE equ (1 << 1)
	PAGE_USER equ (1 << 2)
	PAGE_SUPERVISOR equ (0 << 2)	
	
	PAGE_DIRECTORY_LOW_ENTRY equ (0 >> 22)
	PAGE_DIRECTORY_HIGH_ENTRY equ (KERNEL_VIRTUAL_BASE >> 22)
	PAGE_TABLE_ENTRY equ ((KERNEL_VIRTUAL_BASE >> 12) & 0x3FF)

	;; Paging structures.
	align 4096
	[global kernel_page_directory]
kernel_page_directory:
	times 1024 dd 0
	[global kernel_page_table]
kernel_page_table:
	times 1024 dd 0
	;; A zero frame that we will use for copy on write.
	[global zero_page]
zero_page:
	times 1024 dd 0
	
	;; Export the start symbol so the linker can find it.
	[global start]
start:
	;; Disable interrupts.
	cli
	;; Setup a stack.
	mov esp, (stack_end - KERNEL_VIRTUAL_BASE)
	;; Push the pointer to the multiboot info structure.
	push ebx
	;; Push the multiboot magic number.
	push eax
	;; Initialize the page directory.
	mov ecx, kernel_page_table
	or ecx, (PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)
	;; Page table is mapped in both locations.
	mov [kernel_page_directory + 4 * PAGE_DIRECTORY_LOW_ENTRY], ecx
	mov [kernel_page_directory + 4 * PAGE_DIRECTORY_HIGH_ENTRY], ecx
	;; Map page directory to itself.
	mov ecx, kernel_page_directory
	or ecx, (PAGE_PRESENT | PAGE_SUPERVISOR)
	mov [kernel_page_directory + 4 * 1023], ecx
	;; Initialize the page table.  Map the first 4MB.
	mov eax, 0
	mov ebx, 0x400000
loop1:
	cmp ebx, eax
	jle loop2
	mov ecx, eax
	shr ecx, 12
	and ecx, 0x3FF
	mov edx, eax
	or edx, (PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)
	mov [kernel_page_table + 4 * ecx], edx
	add eax, 0x1000
	jmp loop1
loop2:
	;; Initialize paging.
	mov eax, kernel_page_directory
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	jmp highhalf

	[section .text]
highhalf:	
	;; Setup the stack in order to call kmain.
	add esp, KERNEL_VIRTUAL_BASE
	
	;; Import the kmain symbol.
	[extern kmain]
	call kmain

	[section .bss]
	;; Reserve space for the stack.
	ALIGN STACK_ALIGN
	resb STACK_SIZE
	[global stack_end]
stack_end:	
