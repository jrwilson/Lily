	;; Authors
	;;   Justin R. Wilson

	[bits 32]

	%include "selectors.s"

	[section .text]

%macro EXCEPTION_NOERRCODE 1
	[global exception%1]
exception%1:
	push dword 0
	push dword %1
	jmp exception_common_stub
%endmacro

%macro EXCEPTION_ERRCODE 1
	[global exception%1]
exception%1:
	push dword %1
	jmp exception_common_stub
%endmacro

	;; Import exception_handler.
	[extern exception_handler]
exception_common_stub:
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
	call exception_handler
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
	
EXCEPTION_NOERRCODE 0
EXCEPTION_NOERRCODE 1
EXCEPTION_NOERRCODE 2
EXCEPTION_NOERRCODE 3
EXCEPTION_NOERRCODE 4
EXCEPTION_NOERRCODE 5
EXCEPTION_NOERRCODE 6
EXCEPTION_NOERRCODE 7
EXCEPTION_ERRCODE 8
EXCEPTION_NOERRCODE 9
EXCEPTION_ERRCODE 10
EXCEPTION_ERRCODE 11
EXCEPTION_ERRCODE 12
EXCEPTION_ERRCODE 13
EXCEPTION_ERRCODE 14
EXCEPTION_NOERRCODE 15
EXCEPTION_NOERRCODE 16
EXCEPTION_NOERRCODE 17
EXCEPTION_NOERRCODE 18
EXCEPTION_NOERRCODE 19
EXCEPTION_NOERRCODE 20
EXCEPTION_NOERRCODE 21
EXCEPTION_NOERRCODE 22
EXCEPTION_NOERRCODE 23
EXCEPTION_NOERRCODE 24
EXCEPTION_NOERRCODE 25
EXCEPTION_NOERRCODE 26
EXCEPTION_NOERRCODE 27
EXCEPTION_NOERRCODE 28
EXCEPTION_NOERRCODE 29
EXCEPTION_NOERRCODE 30
EXCEPTION_NOERRCODE 31
