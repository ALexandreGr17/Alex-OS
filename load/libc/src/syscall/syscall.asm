global	read
global	write
global	open
global	close
global	exit
; ##############################################################################

; ##############################################################################
; #																			   #
; #							  read           						           #
; #																			   #
; ##############################################################################
;
;	perform a read syscall 
;
;	args: 
;		1: file descriptor
;		2: output buffer
;		3: size of buffer
;   return:
;       size of the read

read:
    mov edi, [esp + 4]
    mov esi, [esp + 8]
    mov edx, [esp + 12]
    xor eax, eax
    int 0x80
    ret

; ##############################################################################
; #																			   #
; #							    write           						       #
; #																			   #
; ##############################################################################
;
;	perform a write syscall 
;
;	args: 
;		1: file descriptor
;		2: input buffer
;		3: size of buffer
;   return:
;       size of the write
write:
    mov edi, [esp + 4]
    mov esi, [esp + 8]
    mov edx, [esp + 12]
    mov eax, 1
    int 0x80
    ret

; ##############################################################################
; #																			   #
; #							  open           						           #
; #																			   #
; ##############################################################################
;
;	perform a open syscall 
;
;	args: 
;		1: file name
;   return:
;       file descriptor
open:
    mov edi, [esp + 4]
    mov eax, 2
    int 0x80
    ret

; ##############################################################################
; #																			   #
; #							  close           						           #
; #																			   #
; ##############################################################################
;
;	perform a close syscall 
;
;	args: 
;		1: file descriptor
;   return:
;       succces
close:
    mov edi, [esp + 4]
    mov eax, 3
    ret

; ##############################################################################
; #																			   #
; #							  exit           						           #
; #																			   #
; ##############################################################################
;
;	perform an exit syscall 
;
;	args: 
;		1: exit value
exit:
    mov edi, [esp + 4]
    mov eax, 4
    int 0x80
    ret
