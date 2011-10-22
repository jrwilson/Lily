%include "segments.s"
	
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
	mov ax, KERNEL_DATA_SEGMENT
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
	mov ax, KERNEL_DATA_SEGMENT
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
	;; Enable interrupts.
	sti
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
