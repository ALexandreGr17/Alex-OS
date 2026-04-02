global _start

extern log

section .text
bits 32
_start:
    push hello
    call log
    hlt
    jmp _start


section .data
hello: db "Hello", 0
