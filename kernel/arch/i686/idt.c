#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "../../util/binary.h"

typedef struct {
	uint16_t	base_low;
	uint16_t	segment_selector;
	uint8_t		reserved;
	uint8_t		flags;
	uint16_t	base_high;
} __attribute__((packed)) IDTEntry;

typedef struct {
	uint16_t	limit;
	IDTEntry*	ptr;
} __attribute__((packed)) IDTDescriptor;

IDTEntry g_IDT[256];

IDTDescriptor g_IDTDescriptor = {sizeof(g_IDT)-1, g_IDT};


void __attribute__((cdecl)) i686_IDT_Load(IDTDescriptor* descriptor);

void i686_IDT_SetGate(int interrupt, void* base, uint16_t segment_descriptor, uint8_t flags){
	g_IDT[interrupt].base_low = ((uint32_t)base) & 0xFFFF;
	g_IDT[interrupt].segment_selector = segment_descriptor;
	g_IDT[interrupt].flags = flags;
	g_IDT[interrupt].reserved = 0;
	g_IDT[interrupt].base_high = ((uint32_t)base >> 16) & 0xFFFF;
}

void i686_IDT_EnableGate(int interrupt){
	FLAG_SET(g_IDT[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_DisableGate(int interrupt){
	FLAG_UNSET(g_IDT[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_Initialize(){
	i686_IDT_Load(&g_IDTDescriptor);
}
