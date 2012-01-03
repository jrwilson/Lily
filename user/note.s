	[section .note.lily noalloc]
	align 4
	dd b - a		; name size (not including padding)
	dd d - c		; desc size (not including padding)
	dd 0x01234567		; type
a:	db 'NaMe', 0		; name
b:	align 4
c:	dd 0x76543210
	dd 0x89abcdef
d:	align 4
	