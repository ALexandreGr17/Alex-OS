; ##############################################################################
; #																			   #
; #								   i686_outb								   #
; #																			   #
; ##############################################################################
;
;	send 8bit data to an I/O port
;
;	args: 
;		1: I/O port address
;		2: data to send

global	i686_outb
i686_outb:
	[bits	32]
	mov		dx, [esp + 4]
	mov		al, [esp + 8]
	out		dx, al
	ret

; ##############################################################################
; #																			   #
; #								   i686_inb									   #
; #																			   #
; ##############################################################################
;
;	read 8bit data from a I/O port
;
;	args: 
;		1: I/O port address
;	return:
;		al: data from th I/O port

global	i686_inb
i686_inb:
	[bits	32]
	mov		dx, [esp + 4]
	xor		eax, eax
	in		al, dx
	ret

; ##############################################################################
; #																			   #
; #								   i686_outw								   #
; #																			   #
; ##############################################################################
;
;	send 16bit data to an I/O port
;
;	args: 
;		1: I/O port address
;		2: data to send

global	i686_outw
i686_outw:
	[bits	32]
	mov		dx, [esp + 4]
	mov		ax, [esp + 8]
	out		dx, ax
	ret

; ##############################################################################
; #																			   #
; #								   i686_inw									   #
; #																			   #
; ##############################################################################
;
;	read 16bit data from a I/O port
;
;	args: 
;		1: I/O port address
;	return:
;		ax: data from th I/O port

global	i686_inw
i686_inw:
	[bits	32]
	mov		dx, [esp + 4]
	xor		eax, eax
	in		ax, dx
	ret

; ##############################################################################
; #																			   #
; #								   i686_outl								   #
; #																			   #
; ##############################################################################
;
;	send 32bit data to an I/O port
;
;	args: 
;		1: I/O port address
;		2: data to send

global	i686_outl
i686_outl:
	[bits	32]
	mov		dx, [esp + 4]
	mov		eax, [esp + 8]
	out		dx, eax
	ret

; ##############################################################################
; #																			   #
; #								   i686_inl									   #
; #																			   #
; ##############################################################################
;
;	read 32bit data from a I/O port
;
;	args: 
;		1: I/O port address
;	return:
;		eax: data from th I/O port

global	i686_inl
i686_inl:
	[bits	32]
	mov		dx, [esp + 4]
	xor		eax, eax
	in		eax, dx
	ret



; ##############################################################################
; #																			   #
; #								 i686_panic									   #
; #																			   #
; ##############################################################################
;
;	stop the kernel
;
global i686_panic
i686_panic:
	cli
	hlt

; ##############################################################################
; #																			   #
; #								 i686_iowait								   #
; #																			   #
; ##############################################################################
;
;	send data to an unused port to create a delay
;	

%define UNUSED_PORT		0x80
global i686_iowait
i686_iowait:
	push	eax
	xor		eax, eax
	out		UNUSED_PORT, al
	pop		eax
	ret

; ##############################################################################
; #																			   #
; #						   i686_EnableInterrupts							   #
; #																			   #
; ##############################################################################
;
;	enable interrupts with sti
;

global i686_EnableInterrupts
i686_EnableInterrupts:
	sti
	ret

; ##############################################################################
; #																			   #
; #						   i686_DisableInterrupts							   #
; #																			   #
; ##############################################################################
;
;	disbale interrupts with cli
;

global i686_DisableInterrupts
i686_DisableInterrupts:
	cli
	ret


global crash_me
crash_me:
	mov		edx, 0
	div		edx
