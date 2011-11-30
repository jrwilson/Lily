	;; Authors
	;;   Justin R. Wilson

	[bits 32]

	%include "selectors.s"

	[section .text]

	;; Export.
	[global idt_flush]
idt_flush:
	mov eax, [esp+4]	; Get the pointer.
	lidt [eax]
	ret
