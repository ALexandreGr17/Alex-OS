[bits	32]

; ##############################################################################
; #																			   #
; #							   i686_GDT_Load								   #
; #																			   #
; ##############################################################################
;
;	load gdt in mem and update code and data segment
;
;	args: 
;		1: ptr to gdt descriptor
;		2: new code segment
;		3: new data segment

global i686_GDT_Load
i686_GDT_Load:
	push	ebp
	mov		ebp, esp
	
	; load GDT
	mov		eax, [ebp + 8]
	lgdt	[eax]

	; return far to set new code segment
	mov		eax, [ebp + 12]
	push	eax
	push	.reload_cs
	retf

.reload_cs
	; set new data segment
	mov		ax, [ebp + 16]
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax
	mov		ss, ax

	mov		esp, ebp
	pop		ebp
	ret
