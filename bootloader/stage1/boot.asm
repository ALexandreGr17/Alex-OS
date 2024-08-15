org 0x7C00
bits 16

%define	ENDL 0x0D, 0x0A
; ##############################################################################
; #																			   #
; #							FAT12 Header									   #
; #																			   #
; ##############################################################################
jmp short start
nop
bpb_oem:					db	"MSWIN4.1" ; 8 BITS
bpb_bytes_per_sector:		dw	512
bpb_sectors_per_cluster:	db	1
bpb_reseved_sector_count:	dw	1
bpb_fat_count:				db	2
bpb_root_dir_entry_count:	dw	0xE0
bpb_total_sector:			dw	2880		; define in makefile dd comand
bpb_media_descriptor:		db	0xF0		; floppy disk 3.5"
bpb_sectors_per_fat:		dw	9			; 
bpb_sectors_per_track:		dw  18
bpb_head_count:				dw	2
bpb_hidden_sectors:			dd  0
bpb_large_sector_count:		dd  0

ebpb_drive_number:			db	0x0			; useless paramter but 0 -> floppy 0x80 -> hdd
ebpb_flags:					db  0x0
ebpb_signature:				db	0x29
ebpb_volume_sn:				dd	0x31, 0x32, 0x33, 0x34
ebpb_label:					db  "ALEXOS     " ; 11 bytes
ebpb_filesystem_type:		db	"FAT12   "	  ; 8 bytes

times 90-($-$$) db 0

start:
	; mov partition entry from MBR to somewhere else
	mov		ax, PARTITION_ENTRY_SEG
	mov		es, ax
	mov		di, PARTITION_ENTRY_OFF
	
	mov		cx, 16
	rep		movsb

	mov		ax, 0
	mov		ds, ax
	mov		es, ax

	;	setup stack segment
	mov		ss, ax
	mov		sp, 0x7C00

	;	some BIOS mitgh start at 07c00:0000 insted of 0000:07c00
	push	es
	push	word .after
	retf

.after:
	; BIOS should set DL to drive number
	mov		[ebpb_drive_number], dl
	
	; read something from floppy disk
	; checking if extension is present
	stc
	mov		ah, 0x41
	mov		bx, 0x55AA
	int		13h
	jc		.no_extension
	mov		BYTE [extension], 1
.no_extension:

    mov		bx, STAGE2_LOAD_SEGMENT
	mov		es, bx
	mov		bx, STAGE2_LOAD_OFFSET

	mov		si, stage2_pos
.loop:
	mov		eax, [si]		; address LBA
	add		si, 4
	mov		cl, [si]		; size
	inc		si				; next addr

	cmp		eax, 0
	je		.end_loop
	call	disk_read

	xor		ch, ch
	shl		cx, 5
	mov		di, es
	add		di, cx			; updating segment by adding length * 32 = offset + length * 512
	mov		es, di

	jmp		.loop

.end_loop:
	mov		dl, [ebpb_drive_number]
	mov		si, PARTITION_ENTRY_OFF
	mov		di, PARTITION_ENTRY_SEG

	mov		ax, STAGE2_LOAD_SEGMENT
	mov		ds, ax
	mov		es, ax
	; es:bx -> 0x500000

	;mov		si, msg
	;call	puts

	jmp		STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

	jmp		wait_key_and_reboot

	cli
	hlt


floppy_error:
	mov		si, msg_read_failed
	call	puts
	jmp		wait_key_and_reboot

wait_key_and_reboot:
	mov		ah, 0
	int		0x16				; wait fir key presed
	jmp		0FFFFh:0			; jump to begining of BIOS, reboot

.hlt:
	cli
	jmp		.hlt
	
; ##############################################################################
; #																			   #
; #								lba_2_chs									   #
; #																			   #
; ##############################################################################
;	
;	converte LBA adresse to CHS address cylinder -> LBA / (bpb_sectors_per_track * bpb_head_count)
;										head     -> (LBA / bpb_sectors_per_track) % bpb_head_count 
;										sector	 -> LBA % bpb_sectors_per_track + 1
;
;	args: 
;		ax:	LBA address
;	return:
;		cl: Sector
;		ch: Cylinder
;       dh: head

lba_2_chs:
	push	ax
	push	dx
	
	xor		dx, dx
	div		WORD [bpb_sectors_per_track]	; ax -> ax / bpb_sectors_per_track
											; dx -> ax % bpb_sectors_per_track
	inc		dx								; dx -> ax % bpb_sectors_per_track + 1 = Sector
	mov		cx, dx

	xor		dx, dx
	div		WORD [bpb_head_count]			; ax -> ax / bpb_head_count	= Cylinder
											; dx -> ax % bpb_head_count = head
	mov		ch, al
	mov		dh, dl

	pop		ax
	mov		dl, al
	pop		ax
	ret


; ##############################################################################
; #																			   #
; #									puts									   #
; #																			   #
; ##############################################################################
;	
;	print string to screen in tty mode
;
;	args:
;		ds:si: ptr to string

puts:
	push	si
	push	ax
	push	bx
.L1:
	lodsb
	or		al, al
	jz		.done
	mov		ah, 0x0E
	mov		bh, 0
	int		10h
	jmp		.L1
.done:
	pop		bx
	pop		ax
	pop		si
	ret

; ##############################################################################
; #																			   #
; #								reset_disk									   #
; #																			   #
; ##############################################################################
;	
;	reset disk system
;
;	args:
;		dl:	drive number
reset_disk:
	pusha
	mov		ah, 0
	stc
	int		13h
	jc		floppy_error
	popa
	ret


; ##############################################################################
; #																			   #
; #								  disk_read									   #
; #																			   #
; ##############################################################################
;	
;	read a sector on disk
;
;	args:
;		eax: LBA address
;		cl:	nb of sectors
;		dl:	drive number
;		es:bx: output

disk_read:
	push	eax
	push	bx
	push	cx
	push	dx
	push	si
	push	di

	; checking if disk support extension
	cmp		BYTE [extension], 1
	jne		.no_extension
	mov		[dap.sector_count], cl
	mov		[dap.segment], es
	mov		[dap.offset], bx
	mov		[dap.lba], eax

	mov		ah, 0x42
	mov		si, dap
	mov		di, 3
	jmp		.reading_loop

.no_extension:
	push	cx
	call	lba_2_chs
	pop		ax
	mov		ah, 0x02
	mov		di, 3
.reading_loop:
	pusha
	stc
	int		13h
	jnc		.done

	popa
	call	reset_disk
	dec		di
	test	di, di
	jnz		.reading_loop
	jmp		floppy_error
.done:
	popa
	pop		di
	pop		si
	pop		dx
	pop		cx
	pop		bx
	pop		eax
	ret

;
; structur
;	entri for stage2 position
;		4 byte addr
;		1 byte size
;	6 entries


msg_read_failed	db	"read failed", ENDL, 0
msg				db	"here", ENDL, 0
STAGE2_LOAD_SEGMENT		equ	0x0
STAGE2_LOAD_OFFSET		equ 0x500
extension:		db 0
dap:
	.size:			db 0x10
					db 0
	.sector_count:	db 0x00, 0x00
	.offset:		dw 0x0000
	.segment:		dw 0x0000
	.lba:			dq 0

PARTITION_ENTRY_SEG	equ 0x2000
PARTITION_ENTRY_OFF	equ 0x0

times 480-($-$$) db 0x0				 ; making sur that the addr of stage2 is a the end of the file (480 = 512 - 32)
stage2_pos		times	30	db 0x00
dw 0xAA55
buffer:
