	;; Authors
	;;   http://http://wiki.osdev.org/Higher_Half_With_GDT
	;;   Justin R. Wilson

	[bits 32]

	%include "selectors.s"

	[section .text]
	
	;; Export gdt_flush.
	[global gdt_flush]
gdt_flush:
	mov eax, [esp+4]	; Get the pointer.
	;; Load the GDT.
	lgdt [eax]
	;; Change all of the data segments.
	mov ax, KERNEL_DATA_SELECTOR
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	;; Change the code segment by executing a far jump.
	jmp KERNEL_CODE_SELECTOR:gdt_flush2

gdt_flush2:
	ret
