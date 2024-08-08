#ifndef IRQ_H
#define IRQ_H

#include "isr.h"

typedef void (*IRQHandler)(Register* regs);

void i686_IRQ_Initialize();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);

#endif
