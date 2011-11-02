	;; Authors
	;;   http://http://wiki.osdev.org/Higher_Half_With_GDT
	;;   Justin R. Wilson

	[bits 32]

	%include "segments.s"

	[section .text]
	
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

	;; Export.
	[global idt_flush]
idt_flush:
	mov eax, [esp+4]	; Get the pointer.
	lidt [eax]
	ret
