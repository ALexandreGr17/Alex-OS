#include "pic.h"
#include "io.h"
#include <stdint.h>

#define PIC1_COMMAND_PORT		0x20
#define PIC1_DATA_PORT			0x21
#define PIC2_COMMAND_PORT		0xA0
#define PIC2_DATA_PORT			0xA1

// Initialisation Control word 1
// -----------------------------
//  0	IC4		if set, the PIC expect to receive ICW4
//  1	SGNL	if set, only 1 PIC in the system, if unset the PIC is cascaded with slave and ICW3 must be sent
//  2	ADI		ignored on x86 set to 0
//  3   LTIM	if set, operate in level triggered mode, if unset operate in edge triggered mode
//  4	INIT	set to 1 to init the pic
//  5-7			ignored on x86

typedef enum {
	PIC_ICW1_ICW4				= 0x01,
	PIC_ICW1_SINGLE				= 0x02,
	PIC_ICW1_INTERVAL4			= 0x04,
	PIC_ICW1_LEVEL				= 0x08,
	PIC_ICW1_INIT				= 0x10

}PIC_ICW1;

// Initialisation Control word 4
// -----------------------------
//  0	uPM		if set, the PIC is in 80x86 mode, if unset in MCS-80/86 mode
//  1	AEOI	if set, on last interrupt acknowledge puls, controller automatically performs EOI operation
//  2	M/S		only use if BUF is set, if set select buffer master, if unset select buffer slave
//  3   BUF		if set, controller operates in bufferd mode
//  4	SFNM	specially fully nested  mode, used in systems with large number of cascaded controllers
//  5-7			reserved set to 0

typedef enum {
	PIC_ICW4_8086				= 0x01,
	PIC_ICW4_AUTO_EOI			= 0x02,
	PIC_ICW4_BUFFER_MASTER		= 0x04,
	PIC_ICW4_BUFFER_SLAVE		= 0x00,
	PIC_ICW4_BUFFERED			= 0x08,
	PIC_ICW4_SFNM				= 0x10
}PIC_ICW4;

typedef enum {
	PIC_CMD_EOI			= 0x20,
	PIC_CMD_READ_IRR	= 0x0A,
	PIC_CMD_READ_ISR	= 0x0B,
}PIC_CMD;

void i686_PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2){
	// Initialisation control word 1
	i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
	i686_iowait();
	i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
	i686_iowait();

	// Initialisation control word 2
	i686_outb(PIC1_DATA_PORT, offsetPic1);
	i686_iowait();
	i686_outb(PIC2_DATA_PORT, offsetPic2);
	i686_iowait();

	// Initialisation control word 3
	i686_outb(PIC1_DATA_PORT, 0x04); // slave on IRQ2 (pin 3)
	i686_iowait();
	i686_outb(PIC2_DATA_PORT, 0x02); // tel slave it's id (IRQ2)
	i686_iowait();

	// Initialisation control word 4
	i686_outb(PIC1_DATA_PORT, PIC_ICW4_8086); 
	i686_iowait();
	i686_outb(PIC2_DATA_PORT, PIC_ICW4_8086);
	i686_iowait();

	// clear data register
	i686_outb(PIC1_DATA_PORT, 0);
	i686_iowait();
	i686_outb(PIC2_DATA_PORT, 0);
	i686_iowait();
}


void i686_PIC_Mask(int irq){
	if(irq < 8){
		uint8_t mask = i686_inb(PIC1_DATA_PORT);
		i686_outb(PIC1_DATA_PORT, mask | (1 << irq));
	}
	else{
		irq -= 8;
		uint8_t mask = i686_inb(PIC2_DATA_PORT);
		i686_outb(PIC2_DATA_PORT, mask | (1 << irq));
	}
}


void i686_PIC_Unmask(int irq){
	if(irq < 8){
		uint8_t mask = i686_inb(PIC1_DATA_PORT);
		i686_outb(PIC1_DATA_PORT, mask & ~(1 << irq));
	}
	else{
		irq -= 8;
		uint8_t mask = i686_inb(PIC2_DATA_PORT);
		i686_outb(PIC2_DATA_PORT, mask & ~(1 << irq));
	}
}

void i686_PIC_Disable(){
	i686_outb(PIC1_DATA_PORT, 0xFF);
	i686_iowait();
	i686_outb(PIC2_DATA_PORT, 0xFF);
	i686_iowait();
}

void i686_PIC_SendEOI(int irq){
	if(irq >= 8){
		i686_outb(PIC2_COMMAND_PORT, PIC_CMD_EOI);
	}
	i686_outb(PIC1_COMMAND_PORT, PIC_CMD_EOI);
}

uint16_t i686_PIC_ReadIRQRequestRegister(){
	i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
	i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
	return (i686_inb(PIC1_COMMAND_PORT) | (i686_inb(PIC2_COMMAND_PORT) << 8));
}

uint16_t i686_PIC_ReadInServiceRegister(){
	i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
	i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
	return (i686_inb(PIC1_COMMAND_PORT) | (i686_inb(PIC2_COMMAND_PORT) << 8));
}
