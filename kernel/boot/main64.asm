global long_mode_start

extern multiboot_info
extern bootstrap_main

section .bootstrap
bits 64
long_mode_start:

    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov dword [0xb8000], 0x0f340f36

    mov rdi, [multiboot_info]
    call bootstrap_main

    hlt
