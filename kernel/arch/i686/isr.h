#ifndef ISR_H
#define ISR_H

#include <stdint.h>
typedef struct {
	uint32_t ds;											// data segment
	uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;	// pusha
	uint32_t interrupt, error;								// value that we pushed
	uint32_t eip, cs, eflags, esp, ss;						//  pushed by the cpu
} __attribute__((packed)) Register;

typedef void (*ISRHandler)(Register* regs);

void i686_ISR_Initialize();
void i686_ISR_Registerhandler(int interrupt, ISRHandler handler);
#endif
