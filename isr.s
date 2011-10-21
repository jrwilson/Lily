	KERNEL_DATA_SEGMENT equ 0x10
	
	;; Trap Service Routine
%macro TSR_NOERRCODE 1
	[global tsr%1]
tsr%1:
	push dword 0
	push dword %1
	jmp tsr_common_stub
%endmacro

%macro TSR_ERRCODE 1
	[global tsr%1]
tsr%1:
	push dword %1
	jmp tsr_common_stub
%endmacro

	;; Import tsr_handler.
	[extern tsr_handler]
tsr_common_stub:
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
	call tsr_handler
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
	
TSR_NOERRCODE 0
TSR_NOERRCODE 1
TSR_NOERRCODE 2
TSR_NOERRCODE 3
TSR_NOERRCODE 4
TSR_NOERRCODE 5
TSR_NOERRCODE 6
TSR_NOERRCODE 7
TSR_ERRCODE 8
TSR_NOERRCODE 9
TSR_ERRCODE 10
TSR_ERRCODE 11
TSR_ERRCODE 12
TSR_ERRCODE 13
TSR_ERRCODE 14
TSR_NOERRCODE 15
TSR_NOERRCODE 16
TSR_NOERRCODE 17
TSR_NOERRCODE 18
TSR_NOERRCODE 19
TSR_NOERRCODE 20
TSR_NOERRCODE 21
TSR_NOERRCODE 22
TSR_NOERRCODE 23
TSR_NOERRCODE 24
TSR_NOERRCODE 25
TSR_NOERRCODE 26
TSR_NOERRCODE 27
TSR_NOERRCODE 28
TSR_NOERRCODE 29
TSR_NOERRCODE 30
TSR_NOERRCODE 31

	;; Interrupt Service Routine
%macro ISR 2
	[global isr%1]
isr%1:
	push dword 0		; Error code.
	push dword %2		; Interrupt number.
	jmp isr_common_stub
%endmacro

	;; Import isr_handler.
	[extern isr_handler]
isr_common_stub:
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
	call isr_handler
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
	
ISR 0, 32
ISR 1, 33
ISR 2, 34
ISR 3, 35
ISR 4, 36
ISR 5, 37
ISR 6, 38
ISR 7, 39
ISR 8, 40
ISR 9, 41
ISR 10, 42
ISR 11, 43
ISR 12, 44
ISR 13, 45
ISR 14, 46
ISR 15, 47
