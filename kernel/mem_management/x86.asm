global	x86_get_esp
x86_get_esp:
	push	ebp
	mov		ebp, esp

	mov		eax, esp

	mov		esp, ebp
	pop		ebp
	ret
