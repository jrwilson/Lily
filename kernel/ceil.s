	;; From glibc/sysdeps/i386/fpu/s_ceil.S
	
	[bits 32]

	[section .text]
	
	;; Export ceil.
	[global ceil]
ceil:
	fld qword [esp + 4]
	sub esp, 8
	fstcw [esp + 4]
	mov edx, 0x0800
	or edx, [esp + 4]
	and edx, 0xfbff
	mov [esp], edx
	fldcw [esp]
	frndint
	fldcw [esp + 4]
	add esp, 8
	ret
	
