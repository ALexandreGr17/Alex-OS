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
  ;  mov eax, cr4
  ;  or  eax, 0x10
  ;  mov cr4, eax
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax
    ret
