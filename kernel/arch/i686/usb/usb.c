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

void USB_init_xhci_controller(uint16_t id){
	uint16_t size = 0;
	pci_bar_t* bar = pci_get_port_info(id, &size);
	printf("USB nb bar: %i\n", size);
	for (int i = 0; i < size; i++) {
		printf("\tUSB: mode : 0x%x, addr: 0x%x\n", bar[i].type, bar[i].addr);
	}
}

void USB_scan_controller(){
	uint16_t controller_id[MAX_CONTROLLER] = {0};
	uint16_t size = pci_get_device_by_class(0xc, controller_id);
	for(int i = 0; i < size; i++){
		uint8_t info[3] = {0};
		pci_get_device_info(controller_id[i], info);
		if(info[1] == 0x3){
			switch (info[2]) {
				case XHCI:
					USB_init_xhci_controller(i);
					break;

				default:
					break;
			}
		}
	}
}

void USB_init(){
	USB_scan_controller();
}
