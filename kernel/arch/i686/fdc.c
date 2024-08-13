#include "arch/i686/isr.h"
#include <arch/i686/fdc.h>
#include <stdint.h>
#include <arch/i686/io.h>
#include <stdio.h>
#include <arch/i686/irq.h>

uint8_t irq6 = 0;

typedef enum
{
   STATUS_REGISTER_A                = 0x3F0, // read-only
   STATUS_REGISTER_B                = 0x3F1, // read-only
   DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   TAPE_DRIVE_REGISTER              = 0x3F3,
   MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
   DATA_FIFO                        = 0x3F5,
   DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
   CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
} FloppyRegisters;

typedef enum {
	DOR_FLAG_MOTD	= 0x80,		// |
	DOR_FLAG_MOTC	= 0x40,		// |--> control motor of drives A,B,C,D
	DOR_FLAG_MOTB	= 0x20,		// |
	DOR_FLAG_MOTA	= 0x10,		// |
	DOR_FLAG_IRQ	= 0x08,		// control IRQ and DMA
	DOR_FLAG_RESET	= 0x04,		// reset disk
	DOR_FLAG_DSEL	= 0x03,		// select drive number 
} DOR_FLAG;

typedef enum {
	MSR_FLAG_RQM	= 0x80,		// set if OK to exchange bytes with DATA_FIFO register
	MSR_FLAG_DIO	= 0x40,		// set if FIFO IO port expect an IN opcode
	MSR_FLAG_NDMA	= 0x20,		// set in Execution phase of PIO mode read/write command only
	MSR_FLAG_CB		= 0x10,		// command Busy
	MSR_FLAG_ACTD	= 0x08,		// | 
	MSR_FLAG_ACTC	= 0x04,		// |--> Drive A|B|C|D| is seeking
	MSR_FLAG_ACTB	= 0x02,		// |
	MSR_FLAG_ACTA	= 0x01,		// |
}MSR_FLAG;
// 0 0 0 0 0 0 0 0
// 1 1 0 1 0 0 0 0
typedef enum
{
   READ_TRACK =                 2,	// generates IRQ6
   SPECIFY =                    3,      // * set drive parameters
   SENSE_DRIVE_STATUS =         4,
   WRITE_DATA =                 5,      // * write to the disk
   READ_DATA =                  6,      // * read from the disk
   RECALIBRATE =                7,      // * seek to cylinder 0
   SENSE_INTERRUPT =            8,      // * ack IRQ6, get status of last command
   WRITE_DELETED_DATA =         9,
   READ_ID =                    10,	// generates IRQ6
   READ_DELETED_DATA =          12,
   FORMAT_TRACK =               13,     // *
   DUMPREG =                    14,
   SEEK =                       15,     // * seek both heads to cylinder X
   VERSION =                    16,	// * used during initialization, once
   SCAN_EQUAL =                 17,
   PERPENDICULAR_MODE =         18,	// * used during initialization, once, maybe
   CONFIGURE =                  19,     // * set controller parameters
   LOCK =                       20,     // * protect controller params from a reset
   VERIFY =                     22,
   SCAN_LOW_OR_EQUAL =          25,
   SCAN_HIGH_OR_EQUAL =         29
} FloppyCommands;

void irq6_handle(Register* regs){
	irq6 = 1;
}

void wait_irq6(){
	while(!irq6);
}

uint8_t floppy_wait_ready(){
	for(int i = 0; i < 500; i++){
		if(i686_inb(MAIN_STATUS_REGISTER) & MSR_FLAG_RQM){
			return 1;
		}
	}
	return 0;
}

#define CMD_TEST(val) !(val & MSR_FLAG_RQM) //&& !(val & MSR_FLAG_DIO)

uint8_t floppy_send_cmd(uint8_t cmd){
	uint8_t msr_val = i686_inb(MAIN_STATUS_REGISTER);
	if(!CMD_TEST(msr_val)){
		return 0;
	}

	i686_outb(DATA_FIFO, cmd);
	msr_val = i686_inb(MAIN_STATUS_REGISTER);
	while(CMD_TEST(msr_val)){
		msr_val = i686_inb(MAIN_STATUS_REGISTER);
		printf("msr: 0x%x\n", msr_val);
	}

	if(msr_val & MSR_FLAG_DIO){
		printf("Error 1");
		return 0;
	}
	return 1;
}

void floppy_reset(){
	irq6 = 0;
	i686_outb(DIGITAL_OUTPUT_REGISTER, 0x00);
	i686_outb(DIGITAL_OUTPUT_REGISTER, 0x1C);		// set reset bytes and IRQ bytes of DIO and select drive 0

	wait_irq6();

	for(int i = 4; i > 0; i--){
		floppy_send_cmd(SENSE_INTERRUPT);
	}

	i686_outb(CONFIGURATION_CONTROL_REGISTER, 0x00);

	floppy_send_cmd(SPECIFY);
	i686_outb(DATA_FIFO, (8 << 4) | 15);			// SRT = 8 HUT = 15
	i686_outb(DATA_FIFO, (5 << 1) | 0);				// HRT = 5, NDMA = 0 car in DMA mode
	printf("FDC rested\n");
}

void fdc_init(){
	i686_IRQ_RegisterHandler(6, irq6_handle);

	// sending command not the proper way for initialization purpose

	i686_outb(DATA_FIFO, VERSION);

	if(!floppy_wait_ready()){
		printf("Error 2\n");
		return;
	}

	uint8_t version = i686_inb(DATA_FIFO);
	printf("FDC Version: 0x%x\n", version);

	i686_outb(DATA_FIFO, CONFIGURE);
	i686_outb(DATA_FIFO, 0x00);		
	i686_outb(DATA_FIFO, 1 << 6 | 0 << 5 | 0 << 4 | (8-1));	
	i686_outb(DATA_FIFO, 0x00);	

	i686_outb(DATA_FIFO, LOCK);
	floppy_reset();

	for(int i = 0; i < 4; i++){
		floppy_send_cmd(RECALIBRATE);
		i686_outb(DATA_FIFO, i);
		wait_irq6();

		floppy_send_cmd(SENSE_INTERRUPT);
	}

	printf("FDC Configured\n");
}

// 0000 0000
// 0000 1100

void lba_to_chs(uint32_t lba, uint8_t* c, uint8_t* h, uint8_t* s){
	*c = lba / (18 * 2);
	*h = (lba % (2 * 18)) / 18;
	*s = (lba % (2 * 18)) % 18 + 1;
}

void floppy_read(uint32_t lba, uint8_t drive, uint8_t* data_out){
	uint8_t c, h, s;
	lba_to_chs(lba, &c, &h, &s);

	floppy_wait_ready();

	floppy_send_cmd(READ_DATA | 0x80 | 0x40);
	i686_outb(DATA_FIFO, h << 2 | drive);	
	i686_outb(DATA_FIFO, c);
	i686_outb(DATA_FIFO, h);
	i686_outb(DATA_FIFO, s);
	i686_outb(DATA_FIFO, 2);
	i686_outb(DATA_FIFO, 18);
	i686_outb(DATA_FIFO, 0x1B);
	i686_outb(DATA_FIFO, 0xff);

	for(int i = 0; i < 512; i++){
		floppy_wait_ready();
		data_out[i] = i686_inb(DATA_FIFO);
	}
}

