section .bootstrap
bits 64

; #############################################################################
; #                                   bootstrap_io_in8                                  #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (1 byte)
;
global bootstrap_io_in8
bootstrap_io_in8:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in al, dx
    pop rbp
    ret

; #############################################################################
; #                                   bootstrap_io_in16                                 #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (2 byte)
;
global bootstrap_io_in16
bootstrap_io_in16:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in ax, dx
    pop rbp
    ret

; #############################################################################
; #                                   bootstrap_io_in32                                 #
; #############################################################################
;
; args:
;       1: port
; return:
;       data (4 byte)
;
global bootstrap_io_in32
bootstrap_io_in32:
    push rbp
    xor rax, rax
    mov rdx, rdi
    in eax, dx
    pop rbp
    ret


; #############################################################################
; #                                   bootstrap_io_out8                                 #
; #############################################################################
;
; args:
;       1: port
;       2: data (1 byte)
;
global bootstrap_io_out8
bootstrap_io_out8:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, al
    pop rbp
    ret


; #############################################################################
; #                                   bootstrap_io_out16                                #
; #############################################################################
;
; args:
;       1: port
;       2: data (2 byte)
;
global bootstrap_io_out16
bootstrap_io_out16:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, ax
    pop rbp
    ret

; #############################################################################
; #                                   bootstrap_io_out32                                #
; #############################################################################
;
; args:
;       1: port
;       2: data (4 byte)
;
global bootstrap_io_out32
bootstrap_io_out32:
    push rbp
    mov rdx, rdi
    mov rax, rsi
    out dx, eax
    pop rbp
    ret

