extern main
extern exit

global _start
_start:
    call main
    push eax
    call exit

fail_safe:
    jmp fail_safe
