global log

section .bootstrap
bits 32

;
; arg1: fmt
; argn: ...
log:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx

    mov edi,  DWORD [ebp + 8]
    mov ecx, ebp
    add ecx, 12
    mov esi, edi

.L1:
    cmp byte [esi], 0
    je .done
    
    cmp byte [esi], '%'
    je .fmt

    inc esi
    jmp .L1

.fmt:
    mov byte [esi], 0
    call puts
    inc esi
    cmp byte [esi], 's'
    je .print_string

   cmp byte [esi], 'i'
   je .print_int

   cmp byte [esi], 'x'
   je .print_hex

 ;   cmp byte [esi], 'c'
 ;   je .print_char
    jmp .L1
.print_string:
    push edi
    mov edi, DWORD [ecx]
    add ecx, 4
    call puts
    pop edi
    inc edi
    jmp .L1

.print_int:
    push eax
    push edi
    push edx
    push ebx

    mov eax, DWORD [ecx]
    mov edi, buffer
    add ecx, 4
    mov ebx, 10
.L2:
    div ebx
    add dl, '0'
    mov BYTE [edi], dl
    inc edi
    cmp eax, 0
    jnz .L2

    mov BYTE [edi], 0

    mov edi, buffer
    call puts

    pop ebx
    pop edx
    pop edi
    pop eax
    jmp .L1

.print_hex:
    push eax
    push edi
    push edx
    push ebx

    mov eax, DWORD [ecx]
    mov edi, buffer
    add ecx, 4
    mov ebx, 16
.L3:
    div ebx
    cmp edx, 10
    jl .not_hex
    sub edx, 10
    add edx, 'A'
    jmp .put_int_in_buffer
.not_hex:
    add edx, '0'
.put_int_in_buffer:
    mov BYTE [edi], dl
    inc edi
    cmp eax, 0
    jnz .L3

    mov BYTE [edi], 0

    mov edi, buffer
    call puts

    pop ebx
    pop edx
    pop edi
    pop eax
    jmp .L1

.done:
    call puts

    mov esp, ebp
    pop ebp
    ret

puts:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push eax
    mov esi, 0xB8000

.L1:
    cmp byte [edi], 0
    je .done
    
    mov al, byte [edi]
    mov byte [esi + 1], 0x2F
    mov byte [esi], al

    add esi, 2
    inc edi
    jmp .L1
.done:
    pop eax
    pop esi
    pop edi
    mov esp, ebp
    pop ebp
    ret

section .bootstrap_bss nobits
buffer:
    resb 10
