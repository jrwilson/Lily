	;; Authors
	;;   http://http://wiki.osdev.org/Higher_Half_With_GDT
	;;   Justin R. Wilson

	;; This is a 32-bit operating system.
	[BITS 32]
	;; This code goes in the text segment.
	[section .text]
	;; Export the start symbol so the linker can find it.
	[global start]
	;; Import the kmain symbol.
	[extern kmain]

	;; Multiboot constants.
	;; Bootloader should place things on page boundaries.
	MULTIBOOT_PAGE_ALIGN equ 1<<0
	;; Bootloader should provide a memory map.
	MULTIBOOT_MEMORY_INFO equ 1<<1
	MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
	MULTIBOOT_CHECKSUM equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	;; Multiboot header.
	ALIGN 4
multiboot_header:
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

	;; The size of the stack.
	STACK_SIZE equ 0x1000
	
%include "segments.s"
	
	;; The kernel starts here.
start:
	;; Load the global descriptor table (GDT).
	lgdt [trickgdt]
	;; This code assumes the global descriptor table looks like:
	;; Offset  Content
	;; 0x0     Null descriptor
	;; 0x8     Code segment
	;; 0x10    Data segment
	;; Change all of the data segments.
	mov cx, KERNEL_DATA_SEGMENT
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov ss, cx
	;; Change the code segment by executing a far jump.
	;; Jump to offset higherhalf in the segment indexed by 0x08.
	jmp KERNEL_CODE_SEGMENT:higherhalf

higherhalf:
	;; Setup the stack in order to call kmain.
	mov esp, stack + STACK_SIZE
	;; Push the multiboot magic number.
	push eax
	;; Push the multiboot data structure.
	push ebx

	call kmain

	;; Kernel returned.
	cli			; Disable interrupts.
hang:	
	hlt			; Halt the processor.
	jmp hang

	;; Export gdt_flush.
	[global gdt_flush]
gdt_flush:
	mov eax, [esp+4]	; Get a pointer.
	;; Load the GDT.
	lgdt [eax]
	;; This code assumes the global descriptor table looks like:
	;; Offset  Content
	;; 0x0     Null descriptor
	;; 0x8     Code segment
	;; 0x10    Data segment
	;; Change all of the data segments.
	mov ax, KERNEL_DATA_SEGMENT
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	;; Change the code segment by executing a far jump.
	;; Jump to offset flush2 in the segment indexed by 0x08.
	jmp KERNEL_CODE_SEGMENT:gdt_flush2

gdt_flush2:
	ret

	;; The setup section will be identity mapped so the lgdt instruction in start can find trickgdt, gdt, etc.
	;; The kernel will be compiled to exist at a logical address starting at 0xC0100000 which is 1M past 0xC0000000.
	;; This allows the kernel to be mapped into all address spaces without squashing anything.
	;; The kernel will physically be loaded at 1M (see the linker script).
	;; The trick we will use (from Tim Robinson) is to convert all the logical addresses in the 0xC0100000 range to physical addresses in the 0x0010000 range.
	;; The solution is to add 0x40000000 using segmentation:
	;; 0xC0100000 + 0x40000000 = 0x00100000

	;; Export.
	[global idt_flush]
idt_flush:
	mov eax, [esp+4]	; Get the pointer.
	lidt [eax]
	ret

	;; This data goes in the setup section.  See the linker script.
	[section .setup]
trickgdt:
	;; Size of the GDT.
	dw gdt_end - gdt - 1
	;; Address of the GDT.
	dd gdt

gdt:
	;; Null descriptor (required).
	dw 0			; Limit low.
	dw 0			; Base low.
	db 0			; Base middle.
	db 0			; Access.
	db 0			; Granularity.
	db 0			; Base high.
	;; Code segment.
	;; Base:  0x40000000
	;; Limit: 0xFFFFFFFF
	;; Access: present (1), privilege level [ring] 0 (00), memory segment (1), code (1), can execute lower privelege segment (0), segment can be read (1), accessed [set by processor] (0)
	;; Granularity: 1K resolution (1), 32-bit operands (1), always 0 (0), available for use (0), limit high (0xF)
	dw 0xFFFF		; Limit low.
	dw 0			; Base low.
	db 0			; Base middle.
	db 10011010b		; Access.
	db 11001111b		; Granularity.
	db 0x40			; Base high.
	;; Data segment.
	;; Base:  0x40000000
	;; Limit: 0xFFFFFFFF
	;; Access: present (1), privilege level [ring] 0 (00), memory segment (1), data (0), expand up (0), segment can be written (1), accessed [set by processor] (0)
	;; Granularity: 1K resolution (1), 32-bit operands (1), always 0 (0), available for use (0), limit high (0xF)
	dw 0xFFFF		; Limit low.
	dw 0			; Base low.
	db 0			; Base middle.
	db 10010010b		; Access.
	db 11001111b		; Granularity.
	db 0x40			; Base high.

gdt_end:

	[section .bss]

	;; Reserve space for the stack.
	ALIGN 4
	[global stack]
stack:	
	resb STACK_SIZE
