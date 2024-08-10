#include <stdint.h>
#include <arch/i686/isr.h>
#include <boot/bootparams.h>
#include <memory.h>
#include <stdio.h>
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
#include <arch/i686/pci.h>

extern uint8_t __bss_start;
extern uint8_t __end;
void crash_me();

void timer(Register* regs){
	//printf(".");
}

void __attribute__((section(".entry"))) start(boot_parameters_t* bootparams){
	memset(&__bss_start, 0, (&__end) - (&__bss_start));
	clrscr();
	HAL_Initialaize();

	for(int i = 0; i < 16; i++){
		if(i != 1){
			i686_IRQ_RegisterHandler(i, timer);
		}
	}

	printf("Hello world from kernel\n");
	printf("BootDevice: 0x%x\n", bootparams->BootDevice);
	printf("Memory region count: %i\n", bootparams->Memory.region_count);
	for(int i = 0; i < bootparams->Memory.region_count; i++){
		printf("Start: 0x%llx, length: 0x%llx, type: 0x%x\n", 
				bootparams->Memory.regions[i].Begin, 
				bootparams->Memory.regions[i].Length,
				bootparams->Memory.regions[i].Type);
	}

	pci_scan();
	//crash_me();
	//
	
end:
	for(;;);
}
