#ifndef IDT_H
#define IDT_H
#include <stdint.h>

typedef enum {
	IDT_FLAG_GATE_TASK			= 0x05,		// for hardware multi tasking
	IDT_FLAG_GATE_16BIT_INT		= 0x06,		// Interupt gate (addess of the next instruicion is saved and can't be Interupted)
	IDT_FLAG_GATE_16BIT_TRAP	= 0x07,		// Trap gate (address of the current instruction is saved and can be interrupted) (e.g exception)
	IDT_FLAG_GATE_32BIT_INT		= 0x0E,
	IDT_FLAG_GATE_32BIT_TRAP	= 0x0F,

	IDT_FLAG_RING0				= (0 << 5),
	IDT_FLAG_RING1				= (1 << 5),
	IDT_FLAG_RING2				= (2 << 5),
	IDT_FLAG_RING3				= (3 << 5),

	IDT_FLAG_PRESENT			= 0x80,


} IDT_FLAG;



void i686_IDT_Initialize();
void i686_IDT_DisableGate(int interrupt);
void i686_IDT_EnableGate(int interrupt);
void i686_IDT_SetGate(int interrupt, void* base, uint16_t segment_descriptor, uint8_t flags);

#endif
