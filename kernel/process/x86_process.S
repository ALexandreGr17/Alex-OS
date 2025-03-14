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

    .global i686_load_page_dir
i686_save_context:
    pushl %eax
    mov (%esp + 4), %eax

