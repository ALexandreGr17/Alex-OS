#include "hal.h"
#include "memory_management/memory_management.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
#include <arch/i686/pci/pci.h>
#include <arch/i686/fdc.h>
#include <arch/i686/usb.h>
#include <vfs/vfs.h>
#include <arch/i686/acpi.h>

void HAL_Initialaize(boot_parameters_t* bootparams){
	i686_GDT_Initialize();
	i686_IDT_Initialize();
	i686_ISR_Initialize();
	i686_IRQ_Initialize();
	i686_Keyboard_init();
    init_memory_management(bootparams);
	acpi_init();
	PCI_init();
	USB_init();
	//fdc_init();
}
