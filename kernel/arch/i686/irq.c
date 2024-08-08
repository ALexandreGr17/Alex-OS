#include "irq.h"
#include "isr.h"
#include "pic.h"
#include "io.h"
#include <stdio.h>
#include <stdint.h>

#define PIC_REMAP_OFFSET		0x20	// fist 32 INT ar for CPU

IRQHandler g_IRQHandler[16];

void i686_IRQ_Handler(Register* regs){
	int irq = regs->interrupt - PIC_REMAP_OFFSET;

	uint8_t pic_isr	= i686_PIC_ReadInServiceRegister();
	uint8_t pic_irr	= i686_PIC_ReadIRQRequestRegister();

	if(!g_IRQHandler[irq]){
		printf("Unhandled IRQ %d ISR=0x%x IRR=0x%x \n", irq, pic_isr, pic_irr);
	}
	else {
		g_IRQHandler[irq](regs);
	}
	i686_PIC_SendEOI(irq);
}

void i686_IRQ_Initialize(){
	i686_PIC_Configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);
	// register ISR handlers for each or 16 irq lines
	for(int i = 0; i < 16; i++){
		i686_ISR_Registerhandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
	}
	i686_EnableInterrupts();
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler){
	g_IRQHandler[irq] = handler;
}
