#include "arch/interrupts/pic.h"
#include "arch/io.h"
#include "idt.h"
#include <stdint.h>
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1 + 1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)

enum PIC_ICW1 {
    PIC_ICW1_IC4    = (1 << 0),        // if set can recieve ICW4
    PIC_ICW1_SNGL   = (1 << 1),        // if set only 1 PIC in system if cleard PIC cascade mode
    PIC_ICW1_ADI    = (1 << 2), // unused
    PIC_ICW1_LTIM   = (1 << 3), // level triggerd mode (1), edge triggerd mode (0)
    PIC_ICW1_INIT   = (1 << 4),
};

enum PIC_ICW4 {
    PIC_ICW4_uPM    = (1 << 0),
    PIC_ICW4_AEOI   = (1 << 1),
    PIC_ICW4_MS     = (1 << 2),
    PIC_ICW4_BUF    = (1 << 3),
    PIC_ICW4_SFNM   = (1 << 4),
};


void pic_mask(uint8_t irq) {
    if (irq >= 8) {
        irq -= 8;
        uint8_t mask = io_in8(PIC2_DATA);
        io_out8(PIC2_DATA, mask | (1 << irq));
    }
    else {
        uint8_t mask = io_in8(PIC1_DATA);
        io_out8(PIC1_DATA, mask | (1 << irq));
    }
}

void pic_unmask(uint8_t irq) {
    if (irq >= 8) {
        irq -= 8;
        uint8_t mask = io_in8(PIC2_DATA);
        io_out8(PIC2_DATA, mask & ~(1 << irq));
        // cascade
        mask = io_in8(PIC1_DATA);
        io_out8(PIC1_DATA, mask & ~(1 << 2));
    }
    else {
        uint8_t mask = io_in8(PIC1_DATA);
        io_out8(PIC1_DATA, mask & ~(1 << irq));
    }
}


void pic_init() {
    io_out8(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_IC4); // init ICW1 with ICW4 request, cascade mode and edge triggerd mode
    io_wait();
    io_out8(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_IC4); // init ICW1 with ICW4 request, cascade mode and edge triggerd mode
    io_wait();
    io_out8(PIC1_DATA, 0x20);   // set offset for interrupt of PIC1 line to 32
    io_wait();
    io_out8(PIC2_DATA, 0x28); // set offset for interrupt line of PIC2 to 40
    io_wait();
    io_out8(PIC1_DATA, 0x4); // tell PIC1 slave is on IRQ 2
    io_wait();
    io_out8(PIC2_DATA, 0x2); // tell PIC2 its cascade identity
    io_wait();
    io_out8(PIC1_DATA, PIC_ICW4_uPM); // tell PIC to use x86 mode
    io_wait();
    io_out8(PIC2_DATA, PIC_ICW4_uPM); // tell PIC to use x86 mode
    io_wait();

    io_out8(PIC1_DATA, 0);
    io_wait();
    io_out8(PIC2_DATA, 0);
    io_wait();

    io_out8(PIC1_DATA, 0xFF);
    io_out8(PIC2_DATA, 0xFF);
}

void pic_disable() {
    io_out8(PIC1_DATA, 0xFF);
    io_out8(PIC2_DATA, 0xFF);
}

enum PIC_OCW2 {
    PIC_OCW2_EOI    = (1 << 5),
    PIC_OCW2_SL     = (1 << 6),
};

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        io_out8(PIC2_COMMAND, PIC_OCW2_EOI);
    io_out8(PIC1_COMMAND, PIC_OCW2_EOI);
}

enum PIC_OCW3 {
    PIC_OCW3_IRR    = 0x0A,
    PIC_OCW3_ISR    = 0x0B,
};

uint16_t pic_read_irr() {
    io_out8(PIC1_COMMAND, PIC_OCW3_IRR);
    io_out8(PIC2_COMMAND, PIC_OCW3_IRR);
    return (io_in8(PIC2_COMMAND) << 8) | io_in8(PIC1_COMMAND);
}

uint16_t pic_read_isr() {
    io_out8(PIC1_COMMAND, PIC_OCW3_ISR);
    io_out8(PIC2_COMMAND, PIC_OCW3_ISR);
    return (io_in8(PIC2_COMMAND) << 8) | io_in8(PIC1_COMMAND);
}

void pic_enable_irq(uint8_t irq, interrupt_handler handler) {
    idt_set_interrupt(IDT_FROM_IRQ(irq), handler);
    pic_unmask(irq);
}

void pic_disable_irq(uint8_t irq) {
    pic_mask(irq);
    idt_unset_interrupt(IDT_FROM_IRQ(irq));
}
