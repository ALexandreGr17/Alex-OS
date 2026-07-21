global _start
global multiboot_info

extern long_mode_start
extern log

section .bootstrap
bits 32
_start:

    mov esp, stack_top
    mov [multiboot_info], ebx

    call check_multiboot
    call check_cpuid
    call check_long_mode

    push hello
    call log


    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

    hlt
    jmp _start


check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    push multi_boot_error
    call log
    hlt

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    push cpuid_error
    call log
    hlt

check_long_mode:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret

.no_long_mode:
    push long_mode_error
    call log
    hlt


setup_page_tables:
    mov eax, page_table_l3
    or eax, 0b11
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0b11
    mov [page_table_l3], eax

    mov ecx, 0;
.L1:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .L1
    ret

enable_paging:
    mov eax, page_table_l4
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

section .bootstrap_data
hello: db "Hello", 0
multi_boot_error: db "No multiboot", 0
cpuid_error: db "No cpuid", 0
long_mode_error: db "No long mode", 0

section .bootstrap_bss nobits



; Createing PAE for pagination
align 4096
page_table_l4:
    resb 4096

page_table_l3:
    resb 4096

page_table_l2:
    resb 4096


stack_bottom:
    resb 4096 * 4
stack_top:

multiboot_info:
    resb 4



section .bootstrap_rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)

.pointer:
    dw $ - gdt64 - 1
    dq gdt64
