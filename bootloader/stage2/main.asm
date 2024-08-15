bits	16
section	.entry

extern	__bss_start
extern	__end

extern  cstart
global	entry 

entry:
	cli

	; save boot drive
	mov		[g_BootDrive], dl
	mov		[g_partition_off], si
	mov		[g_partition_seg], di
	; set the stack
	mov		ax, ds
	mov		ss, ax
	mov		sp, 0xFFF0
	mov		bp, sp

	; switch to protected mode
	call	EnableA20		; Enable A20 gate
	call	LoadGDT			; Load GDT

	; set protection enable flag in CR0
	mov		eax, cr0
	or		al, 1
	mov		cr0, eax

	; far jump into protected mode
	jmp		dword 08h:.pmode	; code segment is the 2 entry in the DGT which is offset 0x8

.pmode:
	; this is 32bit protected mode
	[bits	32]
	; setting up segment
	mov		ax, 0x10
	mov		ds, ax	; data sgment is the 3 enty in the GDT wich is offset 0x10
	mov		ss, ax

	; clear bss 
	mov		edi, __bss_start
	mov		ecx, __end
	sub		ecx, edi
	mov		al, 0
	cld			; clear direction flag for rep
	rep stosb	; store the value in al in [edi] then do edi + 1
				; rep repete the instruction for ecx times

	; print hello world
	mov		esi, msg
	mov		edi, ScreenBuffer
	cld		; clean direction flag

	xor		edx, edx
	; calculating linear address of partition
	mov		dx, [g_partition_seg]
	shl		edx, 16
	mov		dx, [g_partition_off]

	push	edx
	mov		dl, [g_BootDrive]
	push	edx
	
	call	cstart

	cli
	hlt

.halt:
	jmp		.halt



EnableA20:
	[bits	16]
	call	A20WaitInput
	mov		al, KbdControllerDisableKeyBoard
	out		KbdControllerCommandPort, al	; send command to siable keyboard

	call	A20WaitInput
	mov		al, KbdControllerReadCtrlOutputPort
	out		KbdControllerCommandPort, al

	call	A20WaitOutput
	in		al, KbdControllerDataPort	; reading control output port (flush output buffer)
	push	eax

	call	A20WaitInput
	mov		al, KbdControllerWriteCtrlOutputPort
	out		KbdControllerCommandPort, al

	call	A20WaitInput
	pop		eax
	or		al, 2						; bit 2 = A20 bit
	out		KbdControllerDataPort, al

	; enable Keyboard
	call	A20WaitInput
	mov		al, KbdControllerEnableKeyBoard
	out		KbdControllerCommandPort, al

	call	A20WaitInput
	ret


A20WaitInput:
	[bits	16]
	; wait uintil status bit 2 is 0
	; by reading from command port, we read status byte
	in		al, KbdControllerCommandPort
	test	al, 2
	jnz		A20WaitInput
	ret

A20WaitOutput:
	[bits	16]
	; wait until status bit 1 is 1 so it can ve read
	in		al, KbdControllerCommandPort
	test	al, 1
	jz		A20WaitOutput
	ret


LoadGDT:
	[bits	16]
	lgdt	[g_GDTDesc]
	ret


KbdControllerCommandPort			equ 0x64
KbdControllerDataPort				equ 0x60
KbdControllerDisableKeyBoard		equ	0xAD
KbdControllerEnableKeyBoard			equ	0xAE
KbdControllerReadCtrlOutputPort		equ	0xD0
KbdControllerWriteCtrlOutputPort	equ	0xD1

ScreenBuffer						equ 0xB8000

msg:	db	"Hello world from 32 bit protected mode!", 0

;	GDT into FLAT memory model

g_GDT:		; NULL Descriptor	
			dq	0

			; 32-bit code segment
			dw	0x0FFFF			; limit (bits 0-15) = 0xFFFFFFFF for full 32-bit range
			dw	0				; base (bit 0-15) = 0x0
			db	0				; base (bit 16-23) = 0x0
			db	10011010b		; access (present, (2b)ring 0, code segement, executable, direction 0, readable)
			db  11001111b		; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
			db	0				; base high

			; 32-bit data segment
			dw	0x0FFFF			; limit (bits 0-15) = 0xFFFFFFFF for full 32-bit range
			dw	0				; base (bit 0-15) = 0x0
			db	0				; base (bit 16-23) = 0x0
			db	10010010b		; access (present, (2b)ring 0, data segement, executable 0, direction 0, writable)
			db  11001111b		; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
			db	0				; base high

			; 16-bit code segment
			dw	0x0FFFF			; limit (bits 0-15) = 0xFFFFFFFF for full 32-bit range
			dw	0				; base (bit 0-15) = 0x0
			db	0				; base (bit 16-23) = 0x0
			db	10011010b		; access (present, ring 0, code segement, executable, direction 0, readable)
			db  00001111b		; granularity (1bit pages, 16-bit pmode) + limit (bits 16-19)
			db	0				; base high

			; 16-bit data segment
			dw	0x0FFFF			; limit (bits 0-15) = 0xFFFFFFFF for full 32-bit range
			dw	0				; base (bit 0-15) = 0x0
			db	0				; base (bit 16-23) = 0x0
			db	10010010b		; access (present, (2b)ring 0, data segement, executable 0, direction 0, writable)
			db  00001111b		; granularity (1bit pages, 16-bit pmode) + limit (bits 16-19)
			db	0				; base high

g_GDTDesc:	dw	g_GDTDesc - g_GDT - 1	; limit = sizeof GDT - 1
			dd  g_GDT					; address of GDT


g_BootDrive: db 0
g_partition_seg: dw 0
g_partition_off: dw 0
