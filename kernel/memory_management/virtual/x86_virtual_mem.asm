; ##############################################################################
; #																			   #
; #								   i686_load_page_dir						   #
; #																			   #
; ##############################################################################
;
;	load page directory in cr3
;
;	args: 
;		1: page directory addr

global i686_load_page_dir
i686_load_page_dir:
    [ bits 32 ]
    mov eax, [esp + 4]
    mov cr3, eax
    ret

; ##############################################################################
; #																			   #
; #								   i686_enable_paging						   #
; #																			   #
; ##############################################################################
;
;	enable paging
;

global i686_enable_paging
i686_enable_paging:
    [ bits 32 ]
    mov eax, cr0
    or  eax, 0x80000001
    mov cr0, eax
    ret

; ##############################################################################
; #																			   #
; #								   i686_get_cr2      						   #
; #																			   #
; ##############################################################################
;
;	get cr2 content
;
;	return: 
;		bad address in CR2

global i686_get_cr2
i686_get_cr2:
    mov eax, cr2
    ret
