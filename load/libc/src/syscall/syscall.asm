gloabl write
write:
    mov edi, [esp + 4]
    mov eax, 1
    int 0x80
    ret

global exit
exit:
    mov edi, [esp + 4]
    mov eax, 4
    int 0x80
    ret
