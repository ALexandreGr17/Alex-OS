global log

section .text
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
    add ecx, 8
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

   ; cmp byte [esi], 'i'
   ; je .print_int

  ;  cmp byte [esi], 'x'
  ;  je .print_hex

 ;   cmp byte [esi], 'c'
 ;   je .print_char
    jmp .L1
.print_string:
    push edi
    mov edi, DWORD [ecx]
    call puts
    pop edi
    inc edi
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
