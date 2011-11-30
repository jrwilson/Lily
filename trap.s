	;; Authors
	;;   Justin R. Wilson

	[bits 32]

	%include "selectors.s"

	[section .text]
%macro TRAP 2
	[global trap%1]
trap%1:
	push dword 0
	push dword %2
	jmp trap_common_stub
%endmacro

	;; Import trap_handler.
	[extern trap_handler]
trap_common_stub:
	;; Push the processor state.
	pusha
	mov ax, ds
	;; Push the old data segment.
	push eax
	;; Load the kernel data segment.
	mov ax, KERNEL_DATA_SELECTOR
	mov ds, ax
	mov es, ax
	mov fs, ax
	;; TODO:  What about the stack segment?
	mov gs, ax
	call trap_handler
	;; Pop the old data segment.
	pop eax
	;; Load the old data segment.
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	;; Pop the processor state.
	popa
	;; Pop the interrupt number and error code.
	add esp, 8
	iret

TRAP 0, 128