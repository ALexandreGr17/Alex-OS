#include "stdio.h"
#include <arch/i686/pci/pci.h>
#include <stdint.h>

#define MAX_CONTROLLER 10

enum USB_CONTROLLER_TYPE {
	UHCI = 0x0,
	OHCI = 0x10,
	EHCI = 0x20,
	XHCI = 0x30,
	DEVICE = 0xFE,
	UNKNOW = 0x80,
};

enum XHCI_OP_REG_OFF {
	CAPLENGTH	= 0,
	RSVD		= 1,
	HCIVERSION	= 2,
	HCSPARAMS1	= 4,
	HCSPARAMS2  = 8,
	HCSPARAMS3  = 0xC,
	HCCPARAMS1  = 0x10,
	DBOFF		= 0x14,
	RTSOFF		= 0x18,
	HCCPARMS2	= 0x1C
};

void XHCI_init(PCI_device_t *xhci_device){
	uint8_t *bar = 0;
	for (int i = 0; i < 6; i++){
		if (xhci_device->bars[i].addr != 0){
			bar = (uint8_t*)xhci_device->bars[i].addr;
			break;
		}
	}
	printf("CAPLENTH: 0x%x\n", *((uint8_t*)bar + CAPLENGTH));
}

void USB_init(){
	PCI_device_t filter = {
		.class_code = SERIAL_BUS_CONTROLLER,
		.subclass = 0x3,
	};
	char mask = CLASS_CODE | SUBCLASS;
	int devices_id[10] = { 0 };
	int nb_found = PCI_get_device_by_filter(&filter, mask, devices_id, 10);
	if (nb_found == 0) {
		printf("USB: No controller cound\n");
	}
	else {
		printf("USB: %i contoller found\n", nb_found);
	}
	for (int i = 0; i < nb_found; i++)
	{
		PCI_device_t *device = PCI_get_device_by_id(devices_id[i]);
		printf("Controller 1: ");
		switch (device->prog_if) {
			case 0:
				printf("UHCI\n");
				break;
			case 0x10:
				printf("OHCI\n");
				break;
			case 0x20:
				printf("EHCI\n");
				break;
			case 0x30:
				printf("XHCI\n");
				XHCI_init(device);
				break;
			case 0x80:
				printf("Unspecified\n");
				break;
			case 0xFE:
				printf("USB device\n");
				break;
			default:
				printf("unknow\n");
				break;
		}
	}
}
