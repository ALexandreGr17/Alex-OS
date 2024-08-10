#include "arch/i686/io.h"
#include <arch/i686/pci.h>
#include <stdint.h>
#include <stdio.h>
#define PCI_DATA_PORT		0xCFC
#define PCI_COMMAND_PORT	0xCF8


// Read command
// 31	 -> Enable read
// 30-24 -> reserved
// 23-16 -> bus
// 15-11 -> device
// 10-8  -> function
// 7-2   -> offset
// 1-0   -> should be 0

typedef struct {
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	command;
	uint16_t	status;
	uint8_t		revision_id;
	uint8_t		prog_if;
	uint8_t		subclass;
	uint8_t		class_code;
	uint8_t		cache_line_size;
	uint8_t		latency_timer;
	uint8_t		header_type;
	uint8_t		BIST;
} pci_common_header_t;

typedef struct {
	pci_common_header_t common;
	uint32_t			bar1;
	uint32_t			bar2;
	uint32_t			bar3;
	uint32_t			bar4;
	uint32_t			bar5;
	uint32_t			cardbus_ptr;
	uint16_t			sub_vendor_id;
	uint16_t			sub_id;
	uint32_t			exp_rom_ba;
	uint8_t				capabilities_ptr;
	uint8_t				reserveds[3];
	uint32_t			reserved;
	uint8_t				int_line;
	uint8_t				int_pin;
	uint8_t				min_grant;
	uint8_t				max_latency;
} pci_general_device_hdr;

uint16_t pci_controller_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;

	address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)1 << 31));

	i686_outl(PCI_COMMAND_PORT, address);

	return (uint16_t)((i686_inl(PCI_DATA_PORT) >> ((offset & 2) * 8))  & 0xFFFF);
}

void pci_controller_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value){
	uint32_t address;

	address = (uint32_t)(bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000);

	i686_outl(PCI_COMMAND_PORT, address);
	i686_outl(PCI_DATA_PORT, value);
}

uint8_t device_has_multiple_function(uint16_t bus, uint16_t device){
	return pci_controller_read(bus, device, 0, 0x0E) & (1<<7);
}

void print_device(uint8_t bus, uint8_t device, uint8_t func, pci_common_header_t* device_hdr){
	printf("Device found, bus: 0x%x, device: 0x%x, func: %i\n", bus, device, func);
	printf("vendorid: 0x%x, deviceid: 0x%x, classcode: 0x%x, subclass 0x%x, progif 0x%x\n",
			device_hdr->vendor_id, device_hdr->device_id, device_hdr->class_code, device_hdr->subclass, device_hdr->prog_if);
}

void pci_scan(){
	printf("\nPCI device:\n", PCI_COMMAND_PORT);
	for(int bus = 0; bus < 8; bus++){
		for(int device = 0; device < 32; device++){
			int nb_func = device_has_multiple_function(bus, device) ? 8 : 1;
			for(int func = 0; func < nb_func; func++){
				uint16_t vendor_id = pci_controller_read(bus, device, func, 0);
				if(vendor_id == 0xFFFF){
					continue;
				}
				pci_common_header_t pci_device_hdr = {
					.vendor_id = vendor_id,
					.device_id = pci_controller_read(bus, device, func, 0x2),
					.command = pci_controller_read(bus, device, func, 0x4),
					.status = pci_controller_read(bus, device, func, 0x6),
					.revision_id = pci_controller_read(bus, device, func, 0x8),
					.prog_if = pci_controller_read(bus, device, func, 0x9) >> 8,
					.subclass = pci_controller_read(bus, device, func, 0xA),
					.class_code = pci_controller_read(bus, device, func, 0xB) >> 8,
					.cache_line_size = pci_controller_read(bus, device, func, 0xC),
					.latency_timer = pci_controller_read(bus, device, func, 0xD) >> 8,
					.header_type = pci_controller_read(bus, device, func, 0xE),
					.BIST = pci_controller_read(bus, device, func, 0xF) >> 8,
				};

				print_device(bus, device, func, &pci_device_hdr);
			}
		}
	}
}
