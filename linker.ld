ENTRY(_start)

KERNEL_LMA = 0x00100000;
KERNEL_VMA = 0xFFFFFFFF80000000;

PHDRS
{
    boot PT_LOAD FLAGS(7);   /* RWX */
    text PT_LOAD FLAGS(5);   /* RX  */
    data PT_LOAD FLAGS(6);   /* RW  */
}

SECTIONS
{
    /* =======================================================
       Bootstrap (chargé à 1 Mio)
       ======================================================= */

    . = KERNEL_LMA;

    .boot : ALIGN(16)
    {
        KEEP(*(.multiboot_header))
    } :boot

    .bootstrap : ALIGN(16)
    {
        *(.bootstrap)
    } :boot

    .bootstrap_rodata : ALIGN(16)
    {
        *(.bootstrap_rodata)
    } :boot

    .bootstrap_data : ALIGN(16)
    {
        *(.bootstrap_data)
    } :boot

    .bootstrap_bss (NOLOAD) : ALIGN(4096)
    {
        *(.bootstrap_bss)
    } :boot

    /* Adresse physique où commencera réellement le kernel */
    __kernel_lma = ALIGN(LOADADDR(.bootstrap_bss) + SIZEOF(.bootstrap_bss), 4096);

    /* =======================================================
       Kernel Higher Half
       ======================================================= */

    . = KERNEL_VMA;

    kernel_start = .;

    .text : AT(__kernel_lma)
    {
        *(.text*)
    } :text

    .rodata : ALIGN(16)
    {
        *(.rodata*)
    } :text

    .data : ALIGN(16)
    {
        *(.data*)
    } :data

    .bss (NOLOAD) : ALIGN(4096)
    {
        *(COMMON)
        *(.bss*)
    } :data

    kernel_end = .;

    /* =======================================================
       Symboles utiles
       ======================================================= */

    kernel_phys_start = LOADADDR(.text);
    kernel_phys_end   = LOADADDR(.bss) + SIZEOF(.bss);

    kernel_virt_start = ADDR(.text);
    kernel_virt_end   = ADDR(.bss) + SIZEOF(.bss);

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}
