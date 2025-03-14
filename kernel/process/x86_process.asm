; ##############################################################################
; #																			   #
; #							  i686_save_context						           #
; #																			   #
; ##############################################################################
;
;   save the context of a process
;
;	args: 
;		1: ptr to a process context structure

global i686_save_context
i686_save_context:
    [ bits 32 ]
    push eax
    mov eax, [esp + 4]

    mov [eax + 4], ebx
    mov [eax + 8], ecx
    mov [eax + 0xC], edx
    mov [eax + 0x10], edi
    mov [eax + 0x14], esi
    mov [eax + 0x18], ebp
    mov [eax + 0x24], es
    mov [eax + 0x28], ds
    mov [eax + 0x2C], cs
    mov [eax + 0x30], ss

    mov ebx, eax
    pop eax
    mov [ebx], eax
    mov [ebx + 0x1C], esp
    mov eax, [esp]
    mov [eax + 0x30], eax

    ret

; ##############################################################################
; #																			   #
; #							  i686_load_context						           #
; #																			   #
; ##############################################################################
;
;   load the context of a process
;
;	args: 
;		1: ptr to a process context structure

global i686_load_context
i686_load_context:
    [ bits 32 ]
    push eax
    mov eax, [esp + 4]

    mov ebx, [eax + 0x30]
    mov [esp], ebx

    mov ebx, [eax + 4]
    mov ecx, [eax + 8]
    mov edx, [eax + 0xC]
    mov edi, [eax + 0x10]
    mov esi, [eax + 0x14]
    mov ebp, [eax + 0x18]
    mov es, [eax + 0x24]
    mov ds, [eax + 0x28]
    mov cs, [eax + 0x2C]
    mov ss, [eax + 0x30]

    mov ebx, eax
    mov eax, [ebx]
    mov esp, [ebx + 0x1C]

    ret

