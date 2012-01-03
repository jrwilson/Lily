	# From glibc/sysdeps/i386/fpu/s_ceil.S
	
	.global ceil
ceil:
	fldl 4(%esp)
	sub $8, %esp
	fstcw 4(%esp)
	mov $0x0800, %edx
	or 4(%esp), %edx
	and $0xfbff, %edx
	mov %edx, (%esp)
	fldcw (%esp)
	frndint
	fldcw 4(%esp)
	add $8, %esp
	ret
	
