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
	
	PAGE_DIRECTORY_LOW_ENTRY equ (0 >> 2)
	PAGE_DIRECTORY_HIGH_ENTRY equ (KERNEL_VIRTUAL_BASE >> 22)
	PAGE_TABLE_ENTRY equ ((KERNEL_VIRTUAL_BASE >> 12) & 0x3FF)

	;; Multiboot values.
	[global multiboot_magic]
multiboot_magic:	
	dd 0
	[global multiboot_info]
multiboot_info:
	dd 0
		
	;; Paging structures.
	align 4096
page_directory:
	times 1024 dd 0
page_table:
	times 1024 dd 0

	;; Export the start symbol so the linker can find it.
	[global start]
start:
	;; Disable interrupts.
	cli
	;; Save the multiboot values.
	mov [multiboot_magic], eax
	mov [multiboot_info], ebx
	;; Initialize the page directory.
	mov ecx, page_table
	or ecx, (PAGE_PRESENT | PAGE_WRITABLE)
	;; Page table is mapped in both locations.
	mov [page_directory + 4 * PAGE_DIRECTORY_LOW_ENTRY], ecx
	mov [page_directory + 4 * PAGE_DIRECTORY_HIGH_ENTRY], ecx
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
	or edx, (PAGE_PRESENT | PAGE_WRITABLE)
	mov [page_table + 4 * ecx], edx
	add eax, 0x1000
	jmp loop1
loop2:
	;; Initialize paging.
	mov eax, page_directory
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	jmp highhalf

	[section .text]
highhalf:	
	;; Setup the stack in order to call kmain.
	mov esp, stack_end

	[extern ctors_begin]
	[extern ctors_end]
	mov  ebx, ctors_begin               ; call the constructors
	jmp  .ctors_until_end
.call_constructor:
	call [ebx]
	add  ebx,4
.ctors_until_end:
	cmp  ebx, ctors_end
	jb   .call_constructor
	
	;; Import the kmain symbol.
	[extern kmain]
	call kmain

	[section .bss]
	;; Reserve space for the stack.
	ALIGN STACK_ALIGN
	[global stack_begin]
stack_begin:	
	resb STACK_SIZE
	[global stack_end]
stack_end:	
