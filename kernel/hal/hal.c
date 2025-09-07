#include "hal.h"
#include "memory/management/memory_management.h"
#include <arch/i686/gdt.h>
#include <arch/i686/idt.h>
#include <arch/i686/isr.h>
#include <arch/i686/irq.h>
#include <driver/keyboard/keyboard.h>
#include <driver/pci/pci.h>
#include <driver/fdc/fdc.h>
#include <driver/usb/usb.h>
#include <vfs/vfs.h>
#include <driver/acpi/acpi.h>
#include <driver/pit/pit.h>

void HAL_Initialaize(boot_parameters_t* bootparams){
	i686_GDT_Initialize();
	i686_IDT_Initialize();
	i686_ISR_Initialize();
	i686_IRQ_Initialize();
	i686_Keyboard_init();
    init_memory_management(bootparams);
    PIT_init();
	// acpi_init();
	PCI_init();
	USB_init();
	//fdc_init();
}
