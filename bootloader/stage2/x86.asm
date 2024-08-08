; ##############################################################################
; #																			   #
; #						      x86_EnterRealMode								   #
; #																			   #
; ##############################################################################
;
;	macro that switch from 32-bit protected mode to 16-bit real mode
;

%macro x86_EnterRealMode 0
	[bits	32]
	jmp		word 18h:.pmode16	; jump into 16-bit protected mode
.pmode16:
	[bits	16]
	; disbale protected mode bit in cr0
	mov		eax, cr0
	and		al, ~1
	mov		cr0, eax

	jmp		word 00h:.rmode		; jump into 16-bit real mode
.rmode:
	; setup segment
	mov		ax, 0
	mov		ds, ax
	mov		ss, ax
	sti		; enable interupt
%endmacro

; ##############################################################################
; #																			   #
; #						   x86_EnterProtectedMode							   #
; #																			   #
; ##############################################################################
;
;	macro that switch from 16-bit real mode mode to 32-bit protected mode
;

%macro x86_EnterProtectedMode 0
	[bits	16]
	cli
	mov		eax, cr0
	or		al, 1
	mov		cr0, eax

	jmp		dword 08h:.pmode
.pmode:
	[bits	32]
	mov		ax, 0x10
	mov		ds, ax
	mov		ss, ax

%endmacro

; ##############################################################################
; #																			   #
; #						     LinearToSegOffset								   #
; #																			   #
; ##############################################################################
;
;	macro that convert a linear 32-bit address to segment:offset address
;
;	args: 
;		%1:	Linear address
;		%2: (out) target segment
;		%3: (out) target 32-bit register to use
;		%4: (out) target lower 16-bit half of %3

%macro LinearToSegOffset 4
	mov		%3, %1		; linear address to eax
	shr		%3, 4
	mov		%2, %4
	mov		%3, %1
	and		%3, 0xF

%endmacro

; ##############################################################################
; #																			   #
; #								   x86_outb									   #
; #																			   #
; ##############################################################################
;
;	send data to an I/O port
;
;	args: 
;		1: I/O port address
;		2: data to send

global	x86_outb
x86_outb:
	[bits	32]
	mov		dx, [esp + 4]
	mov		al, [esp + 8]
	out		dx, al
	ret

; ##############################################################################
; #																			   #
; #								   x86_inb									   #
; #																			   #
; ##############################################################################
;
;	read data from a I/O port
;
;	args: 
;		1: I/O port address
;	return:
;		al: data from th I/O port

global	x86_inb
x86_inb:
	[bits   32]
	mov		dx, [esp + 4]
	xor		eax, eax
	in		al, dx
	ret

; ##############################################################################
; #																			   #
; #								x86_Disk_GetDriveParams						   #
; #																			   #
; ##############################################################################
;
;	get disk drive param from BIOS
;
;	args: 
;		1: disk drive
;		2: (out) drive type
;		3: (out) number of cylinders
;		4: (out) number of sectors
;		5: (out) number of heads
;	return:
;		eax: success

global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
	[bits	32]

	push	ebp
	mov		ebp, esp

	x86_EnterRealMode
	[bits	16]

	push	es
	push	bx
	push	esi
	push	di

	; calling int 13
	mov		dl, [bp + 8] ; arg 1 disk drive
	mov		ah, 0x08
	mov		di, 0x0		; set es:di to 0000:0000
	mov		es, di
	stc
	int		13h

	; out prams
	mov		eax, 1
	sbb		eax, 0

	; drive type from bl
	LinearToSegOffset [bp + 12], es, esi, si
	mov		[es:si], bl

	; cylinders
	mov		bl, ch	; cylinders - lower bits in ch
	mov		bh, cl  ; cylinder - upper bits in cl (6-7)
	shr		bh, 6
	inc		bx

	LinearToSegOffset [bp + 16], es, esi, si
	mov		[es:si], bx

	; sectors
	xor		ch, ch	; sectors - lower 5 bits in cl
	and		cl, 0x3F

	LinearToSegOffset [bp + 20], es, esi, si
	mov		[es:si], cx
	
	; heads
	mov		cl, dh	; heads- dh
	inc		cx

	LinearToSegOffset [bp + 24], es, esi, si
	mov		[es:si], cx

	; restore regs
	pop		di
	pop		esi
	pop		bx
	pop		es

	push	eax
	x86_EnterProtectedMode
	[bits	32]
	pop		eax

	mov		esp, ebp
	pop		ebp
	ret

; ##############################################################################
; #																			   #
; #							 x86_Disk_Reset									   #
; #																			   #
; ##############################################################################
;	
;	reset disk system
;
;	args:
;		1: drive number
;	return:
;		eax: success

global x86_Disk_Reset:
x86_Disk_Reset:
	push	ebp
	mov		ebp, esp

	x86_EnterRealMode
	mov		ah, 0
	mov		dl, [bp + 8]
	stc
	int		13h

	mov		eax, 1
	sbb		eax, 0		; 1 on success, 0 on fail

	push	eax
	x86_EnterProtectedMode
	pop		eax

	mov		esp, ebp
	pop		ebp
	ret

; ##############################################################################
; #																			   #
; #							  x86_Disk_Read									   #
; #																			   #
; ##############################################################################
;	
;	read a sector on the disk
;
;	args:
;		1:	drive
;		2:	Cylinder		|
;		3:	head			| -> CHS value
;		4:  sector			|
;		5:	number of sector to read
;		6:	(out) output buffer
;	return:
;		eax: success

global x86_Disk_Read
x86_Disk_Read:
	push	ebp
	mov		ebp, esp

	x86_EnterRealMode

	push	ebx
	push	es
	push	dx
	push	cx

	; setting up for the interupt
	;	cl[0-5] -> sector
	;	cx[cl[6-7] + ch] -> cylinder
	;	dh -> head
	;	dl -> drive
	;	al -> number of sector
	;	ah -> 0x02
	;	es:bx	-> output buffer

	mov		dl, [bp + 8]
	
	mov		ch, [bp + 12]	; ch - cylinder (lower 8 bits)
	mov		cl, [bp + 13]	; cl - cylinder to bits 6-7
	shl		cl, 6

	mov		al, [bp + 20]	; cl - sectors to bits 0-5
	and		al, 0x3F
	or		cl, al

	mov		dh, [bp + 16]	; dh - head
	
	mov		al, [bp + 24]

	LinearToSegOffset [bp + 28], es, ebx, bx
	mov		ah, 0x02
	stc
	int		13h

	;	return value
	mov		eax, 1
	sbb		eax, 0

	pop		cx
	pop		dx
	pop		es
	pop		ebx

	push	eax

	x86_EnterProtectedMode

	pop		eax
	
	mov		esp, ebp
	pop		ebp
	ret

