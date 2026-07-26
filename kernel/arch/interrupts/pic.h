#ifndef PIC_H
#define PIC_H

#include <stdint.h>
#include "idt.h"

#define IDT_FROM_IRQ(irq) (irq + 0x20)
#define IRQ_FROM_IDR(intr) (intr - 0x20)

void pic_init();
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);
void pic_disable();
void pic_send_eoi(uint8_t irq);
uint16_t pic_read_irr();
uint16_t pic_read_isr();
void pic_enable_irq(uint8_t irq, interrupt_handler handler);
void pic_disable_irq(uint8_t irq);

#endif
