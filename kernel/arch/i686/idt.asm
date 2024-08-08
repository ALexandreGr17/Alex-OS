[bits	32]

; ##############################################################################
; #																			   #
; #							   i686_IDT_Load								   #
; #																			   #
; ##############################################################################
;
;	load IDT in memeory
;
;	args: 
;		1: ptr to idt descriptor

global	i686_IDT_Load
i686_IDT_Load:
	push	ebp
	mov		ebp, esp

	; load IDT
	mov		eax, [ebp + 8]
	lidt	[eax]

	mov		esp, ebp
	pop		ebp
	ret

