#include "hal.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
#include <arch/i686/pci/pci.h>
#include <arch/i686/fdc.h>

void HAL_Initialaize(){
	i686_GDT_Initialize();
	i686_IDT_Initialize();
	i686_ISR_Initialize();
	i686_IRQ_Initialize();
	i686_Keyboard_init();
	//fdc_init();
	pci_scan();
}
