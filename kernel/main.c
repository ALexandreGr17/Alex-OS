#include <stdint.h>
#include <arch/i686/isr.h>
#include <memory.h>
#include <stdio.h>
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
extern uint8_t __bss_start;
extern uint8_t __end;
void crash_me();

void timer(Register* regs){
	//printf(".");
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive){
	memset(&__bss_start, 0, (&__end) - (&__bss_start));
	clrscr();
	HAL_Initialaize();

	for(int i = 0; i < 16; i++){
		if(i != 1){
			i686_IRQ_RegisterHandler(i, timer);
		}
	}

	printf("Hello world from kernel\n");
	//crash_me();
	//
	
end:
	for(;;);
}
