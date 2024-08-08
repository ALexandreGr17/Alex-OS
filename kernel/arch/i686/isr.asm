[bits	32]

%macro ISR_NOERRORCODE 1
global	i686_ISR%1:
i686_ISR%1:
	push	0
	push	%1
	jmp		isr_common
%endmacro

%macro ISR_ERRORCODE 1
global i686_ISR%1
i686_ISR%1:
	push	%1
	jmp		isr_common
%endmacro

%include "arch/i686/isrs_gen.inc"
extern	i686_ISR_Handler
isr_common:
	pusha				; push edi, esi, ebp, esp, ebx, edx, ecx, eax

	xor		eax, eax	; save ds
	mov		ax, ds
	push	eax

	; use kernel data segment
	mov		ax, 0x10
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax

	push	esp			; pass pointer to srack to c function
	call	i686_ISR_Handler
	add		esp, 4		; skip last push

	pop		eax
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax

	popa
	add		esp, 8		; remove error code and interrupt number
	iret
