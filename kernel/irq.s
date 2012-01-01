	;; Authors
	;;   Justin R. Wilson

	[bits 32]

	%include "selectors.s"

	[section .text]

%macro IRQ 2
	[global irq%1]
irq%1:
	push dword 0		; Error code.
	push dword %2		; Interrupt number.
	jmp irq_common_stub
%endmacro

	;; Import irq_handler.
	[extern irq_handler]
irq_common_stub:
	;; Push the processor state.
	pusha
	;; Push the old data segment.
	mov ax, ds
	push eax
	;; Load the kernel data segment.
	mov ax, KERNEL_DATA_SELECTOR
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	;; TODO:  What about the stack segment?
	call irq_handler
	;; Pop the old data segment.
	pop eax
	;; Load the old data segment.
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	;; Pop the processor state.
	popa
	;; Pop the error and interrupt number.
	add esp, 8
	iret
	
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
