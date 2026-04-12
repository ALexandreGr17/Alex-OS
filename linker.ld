ENTRY(_start)

SECTIONS {
    . = 1M;
    kernel_start = .;
    .boot : {
        KEEP(*(.multiboot_header))
    }

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    kernel_end = .;
}
