section .text
bits 64

; #############################################################################
; #                                   io_in8                                  #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (1 byte)
;
global io_in8
io_in8:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in al, dx
    pop rbp
    ret

; #############################################################################
; #                                   io_in16                                 #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (2 byte)
;
global io_in16
io_in16:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in ax, dx
    pop rbp
    ret

; #############################################################################
; #                                   io_in32                                 #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (4 byte)
;
global io_in32
io_in32:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in eax, dx
    pop rbp
    ret


; #############################################################################
; #                                   io_out8                                 #
; #############################################################################
;
; args:
;       1: port
;       2: data (1 byte)
;
global io_out8
io_out8:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, al
    pop rbp
    ret


; #############################################################################
; #                                   io_out16                                #
; #############################################################################
;
; args:
;       1: port
;       2: data (2 byte)
;
global io_out16
io_out16:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, ax
    pop rbp
    ret

; #############################################################################
; #                                   io_out32                                #
; #############################################################################
;
; args:
;       1: port
;       2: data (4 byte)
;
global io_out32
io_out32:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, eax
    pop rbp
    ret

; #############################################################################
; #                                   io_wait                                 #
; #############################################################################

global io_wait
io_wait:
    push rbp
    push rax
    xor al, al
    out 0x80, al
    pop rax
    pop rbp
    ret
