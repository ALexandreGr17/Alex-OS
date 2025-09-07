; ##############################################################################
; #																			   #
; #							  context_switch						           #
; #																			   #
; ##############################################################################
;
;	switch the context
;
;	args: 
;		1: context structure for the old context
;		2: context structure for the new context
;		3: exit value
;   return:
;       return the exit value of the process

global context_switch
context_switch:
    ; recupere la valeur de sortie
    ; Sauvegarder l'ancien contexte
    mov eax, [esp + 4]    ; struct context *old
    mov [eax], esp
    mov [eax + 4], ebp
    mov [eax + 8], ebx
    mov [eax + 12], esi
    mov [eax + 16], edi
    ; Sauver eip (adresse de retour)
    mov ecx, [esp]
    mov [eax + 20], ecx
    mov ecx, [esp + 0xc]

    ; Restaurer le nouveau contexte
    mov eax, [esp + 8]    ; struct context *new
    mov esp, [eax]
    mov ebp, [eax + 4]
    mov ebx, [eax + 8]
    mov esi, [eax + 12]
    mov edi, [eax + 16]
    mov eax, ecx
    ret
