#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include "io.h"
#include <stddef.h>
#include "../../stdio.h"

ISRHandler g_ISRHandlers[256];

static const char* const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};


void i686_ISR_InitializeGates();

void i686_ISR_Initialize(){
	i686_ISR_InitializeGates();
	for(int i = 0; i < 255; i++){
		i686_IDT_EnableGate(i);
	}
}
void __attribute__((cdecl)) i686_ISR_Handler(Register* regs){
	if(g_ISRHandlers[regs->interrupt] != NULL){
		g_ISRHandlers[regs->interrupt](regs);
	}
	else if(regs->interrupt >= 32){
		printf("Unhandled interrupt %d!\n", regs->interrupt);
	}
	else {
		printf("Unhandled Exception %d %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
		printf("	eax=0x%x	ebx=0x%x	ecx=0x%x	edx=0x%x	esi=0x%x	edi=0x%x\n", 
				regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
		printf("	esp=0x%x    ebp=0x%x	eip=0x%x	eflags=0x%x		cs=0x%x		ds=0x%x     ss=0x%x\n",
				regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);
		printf("	interrupt=0x%X		errorcode=0x%x\n", regs->interrupt, regs->error);
		printf("KERNEL PANIC\n");
        i686_panic();
	}
}

void i686_ISR_Registerhandler(int interrupt, ISRHandler handler){
    g_ISRHandlers[interrupt] = handler;
    i686_IDT_EnableGate(interrupt);
}
