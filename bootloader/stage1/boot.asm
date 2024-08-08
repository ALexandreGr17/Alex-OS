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

start:
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
	; read something from floppy disk
    ; BIOS should set DL to drive number
	mov		[ebpb_drive_number], dl

	; calculate address of root dir LBA = bpb_sectors_per_fat * bpb_fat_count + bpb_reseved_sector_count
	mov		ax , [bpb_sectors_per_fat]
	mov		bx, [bpb_fat_count]
	xor		bh, bh
	mul		bx
	add		ax, [bpb_reseved_sector_count]	; ax = LBA of root dir
	
	push	ax

	; compute size of root dir (sart of data section) = 32 * bpb_root_dir_entry_count / bpb_bytes_per_sector
	mov		ax, 32
	mul		WORD [bpb_root_dir_entry_count]
	xor		dx, dx
	div		WORD [bpb_bytes_per_sector]	;	ax = size of root dir
	test	dx, dx
	jz		.good_size
	inc		ax						; handleing case where 32 * bpb_root_dir_entry_count % bpb_bytes_per_sector != 0
.good_size:
	mov		[root_dir_end], ax
	;	load the root dir in memory
	mov		cl, al
	pop		ax
	add		[root_dir_end], ax		; computing the root dir end
	mov		dl, [ebpb_drive_number]
	mov		bx, buffer
	call	read_sector


	;	search test file
	mov		di, buffer
;	mov		si, test_file
	mov		si, stage2_file
	call	find_file
	push	ax

	;	load the FAT in memory
	mov		ax, [bpb_reseved_sector_count]
	mov		bx, buffer
	mov		cl, [bpb_sectors_per_fat]
	mov		dl, [ebpb_drive_number]
	call	read_sector

	mov		bx, KERNEL_LOAD_SEGMENT
	mov		es, bx
	mov		bx, KERNEL_LOAD_OFFSET
	pop		ax
	call	read_file

	;mov		si, KERNEL_LOAD_SEGMENT
	;mov		ds, si
	;mov		si, KERNEL_LOAD_OFFSET
	;call	puts

	mov		dl, [ebpb_drive_number]
	mov		ax, KERNEL_LOAD_SEGMENT
	mov		ds, ax
	mov		es, ax

	jmp		KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

	jmp		wait_key_and_reboot

	cli
	hlt

floppy_error2:
	mov		si, msg_read2_failed
	call	puts
	jmp		wait_key_and_reboot
	
floppy_error:
	mov		si, msg_read_failed
	call	puts
	jmp		wait_key_and_reboot

stage2_not_found_error:
	mov		si, msg_stage2_not_found
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
	jc		floppy_error2
	popa
	ret


; ##############################################################################
; #																			   #
; #								read_sector									   #
; #																			   #
; ##############################################################################
;	
;	read a sector on disk
;
;	args:
;		ax:	LBA address
;		cl:	nb of sectors
;		dl:	drive number
;		es:bx: output

read_sector:
	push	ax
	push	bx
	push	cx
	push	dx
	push	di

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
	pop		dx
	pop		cx
	pop		bx
	pop		ax
	ret


; ##############################################################################
; #																			   #
; #								find_file									   #
; #																			   #
; ##############################################################################
;	
;	find a file in root dir
;
;	args:
;		ds:si:	filename
;		es:di:	root_dir
;	return:
;		ax: first cluster
find_file:
	push	si
	push	di
	xor		bx, bx
.L1:
	push	si
	push	di
	mov		cx, 11
	repe	cmpsb
	
	pop		di
	pop		si

	jz		.done

	add		di, 32
	inc		bx
	cmp		bx, [bpb_root_dir_entry_count]
	jl		.L1
	jmp		stage2_not_found_error
.done:
	mov		ax, [di + 0x1A]
	pop		di
	pop		si
	ret

; ##############################################################################
; #																			   #
; #								read_file									   #
; #																			   #
; ##############################################################################
;	
;	read a file in root dir
;
;	args:
;		ax:		first cluster
;		es:bx:	output

read_file:
	push	ax
	push	bx
	push	dx
	push	cx
.L1:
	

	push	ax
	sub		ax, 2
	xor		dx, dx
	;mul		[bpb_sectors_per_cluster]
	add		ax, [root_dir_end]
	mov		cl, [bpb_sectors_per_cluster]
	mov		dl, [ebpb_drive_number]
	call	read_sector
	
	add		bx, [bpb_bytes_per_sector] ; bpb_sectors_per_cluster = 1
	pop		ax
	mov		cx, 3
	mul		cx
	mov		cx,	2
	xor		dx, dx
	div		cx

	mov		si, buffer
	add		si, ax
	mov		ax, [ds:si]
	test	dx, dx
	jz		.even
	shr		ax, 4
	jmp		.test_finish
.even:
	and		ax, 0x0FFF
.test_finish:
	cmp		ax, 0x0FF8
	jl		.L1
	pop		cx
	pop		ax
	pop		bx
	pop		dx
	ret
msg_read2_failed:	db	"reset failed" ENDL, 0
msg_stage2_not_found:	db	"file not found", ENDL, 0
msg_read_failed	db	"read failed", ENDL, 0
;test_file:		db  "TEST    TXT"
stage2_file:	db  "STAGE2  BIN"
;msg_test:		db	"Hello world!", 0x0d, 0x0a, 0
fat:			db	1
root_dir_end:	db	1
KERNEL_LOAD_SEGMENT		equ	0x0
KERNEL_LOAD_OFFSET		equ 0x500

times 510-($-$$) db 0x0
dw 0xAA55
buffer:
