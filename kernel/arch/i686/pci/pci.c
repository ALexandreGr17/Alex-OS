#include "arch/i686/io.h"
#include <arch/i686/pci/pci.h>
#include <stdint.h>
#include <stdio.h>

#define PCI_DATA_PORT		0xCFC
#define PCI_COMMAND_PORT	0xCF8

#define MAX_PCI_DEVICE 8 * 32 * 8

// Read command
// 31	 -> Enable read
// 30-24 -> reserved
// 23-16 -> bus
// 15-11 -> device
// 10-8  -> function
// 7-2   -> offset
// 1-0   -> should be 0

hardware_pci_device_t devices[MAX_PCI_DEVICE];
uint16_t nb_pci_device = 0;

typedef enum {
	BAR_SIZE_16 = 1,
	BAR_SIZE_32 = 0,
	BAR_SIZE_64 = 2,
}BAR_SIZE;

typedef enum {
	BAR_TYPE_MEM = 0,
	BAR_TYPE_IO = 1,
} BAR_TYPE;

typedef enum {
	PCI_DEVICE_HEADER_TYPE_GENERAL = 0,
	PCI_DEVICE_HEADER_TYPE_PCI_2_PCI_BRIDGE = 0x80,
	PCI_DEVICE_HEADER_TYPE_PCI_2_CARDBUS_BRIDGE = 2,
} PCI_DEVICE_HEADER_TYPE;

uint16_t PCI_controller_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = bus;
	uint32_t lslot = slot;
	uint32_t lfunc = func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)1 << 31));

	i686_outl(PCI_COMMAND_PORT, address);

	return (uint16_t)((i686_inl(PCI_DATA_PORT) >> ((offset & 2) * 8))  & 0xFFFF);
}

void PCI_controller_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value){
	uint32_t address;

	address = (uint32_t)(bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000);

	i686_outl(PCI_COMMAND_PORT, address);
	i686_outl(PCI_DATA_PORT, value);
}

uint32_t PCI_controller_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address;
	uint32_t lbus = bus;
	uint32_t lslot = slot;
	uint32_t lfunc = func;

	address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)1 << 31));

	i686_outl(PCI_COMMAND_PORT, address);

	return (uint16_t)((i686_inl(PCI_DATA_PORT) >> ((offset & 2) * 8))  & 0xFFFF);
}

uint8_t device_has_multiple_function(uint16_t bus, uint16_t device){
	return PCI_controller_read_word(bus, device, 0, 0x0E) & (1<<7);
}

void print_device(hardware_pci_device_t* device){
	printf("PCI Device: \nvendor_id: 0x%x, device_id: 0x%x, class: 0x%x, subclass: 0x%x, prog_if: 0x%x\n",
			device->vendor_id, device->device_id, device->class_code, device->subclass, device->prog_if);
}

uint8_t get_highest_bit_set(uint32_t val){
	uint8_t pos = 31;
	uint32_t mask = 0x80000000;
	while(!(val & mask)){
		pos--;
		mask = mask >> 1;
	}
	return pos;
}

uint8_t get_lowest_bit_set(uint32_t val){
	uint8_t pos = 0;
	uint32_t mask = 1;
	while(!(val & mask)){
		pos++;
		mask = mask << 1;
	}
	return pos;
}

uint8_t handle_32_bit_bar_mem(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, hardware_pci_device_t* device){
	if(device->class_code == 0x3){
		printf("offset: 0x%x\n", offset);
		printf("bus: 0x%x\n", bus);
		printf("slot: 0x%x\n", slot);
		printf("func: 0x%x\n", func);
	}

	device->command = PCI_controller_read_word(bus, slot, func, 0x4);
	if((device->command & 0x2) == 0){
		printf("Enable memory space\n");
		PCI_controller_write(bus, slot, func, 0x4, device->command | 0x2);
	}

	uint32_t original_bar_value = PCI_controller_read_word(bus, slot, func, offset);
	printf("Original BAR value: 0x%x\n", original_bar_value);
	if(original_bar_value == 0){
		device->status = PCI_controller_read_word(bus, slot, func, 0x6);
		printf("status: 0x%x\n", device->status);
		printf("INFO: BAR is not implemented or unused\n");
		return 1;
	}

	PCI_controller_write(bus, slot, func, offset, 0xFFFFFFF0);
	const uint32_t bar_value = PCI_controller_read_word(bus, slot, func, offset) & 0xFFFFFFF0;

	printf("BAR value after masking: 0x%x\n", bar_value);

	if(bar_value == 0){
		device->status = PCI_controller_read_word(bus, slot, func, 0x6);
		printf("status: 0x%x\n", device->status);
		printf("ERROR: 32-bit memory BAR has no writable address bits!\n");
		return 0;
	}

	uint32_t size = (~bar_value & 0xFFFFFFF0) + 1;
	if ((size & (size - 1)) != 0) {
		device->status = PCI_controller_read_word(bus, slot, func, 0x6);
		printf("status: 0x%x\n", device->status);
		printf("ERROR: BAR size is not a power of two!\n");
		return 0;
	}
	PCI_controller_write(bus, slot, func, offset, original_bar_value);
	printf("32-bit memory BAR size: 0x%x bytes.\n", size);
	return 1;
}


void PCI_set_bar_mem(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, hardware_pci_device_t* device){

	device->command = PCI_controller_read_word(bus, slot, func, 0x4);
    if ((device->command & 0x2) == 0) {
        printf("Enable memory space\n");
        PCI_controller_write(bus, slot, func, 0x4, device->command | 0x2);
    }
	uint32_t bar_value = PCI_controller_read_long(bus, slot, func, offset);
	printf("bar_value: 0x%x\n", bar_value);
	switch (bar_value & 0x00000006) {
		case 0:
			// 32 Bits
			printf("bar type: 32\n");
			PCI_controller_write(bus, slot, func, offset, 0xFFFFFFF0);
			uint32_t size = PCI_controller_read_long(bus, slot, func, offset) & 0xFFFFFFF0;
			if(size == 0){
				printf("No writable address bits\n");
				break;
			}
			printf("bar size: 0x%x\n", size);
			break;
		case 4:
			printf("bar type: 64\n");
			// 64 Bits
			break;
		case 2:
			printf("bar type: 20\n");
			// 20 Bits
			break;
	}
		
}

void PCI_get_bar(uint8_t bus, uint8_t slot, uint8_t func, hardware_pci_device_t* device){
	for(int i = 0; i < device->nb_bar; i++){
		printf("	---------------------- bar --------------------\n");
		uint32_t bar_value = PCI_controller_read_long(bus, slot, func, 0x10 + (i * 4));
		printf("bar_value: 0x%x\n", bar_value);
		if((bar_value & 1) == BAR_TYPE_IO){
			//PCI_set_bar_io();
		}
		else {
			if(bar_value != 0){
				PCI_set_bar_mem(bus, slot, func, 0x10 + (i * 4), device);
			}
		}

		printf("\n\n");
		PCI_controller_write(bus, slot, func, 0x10 + (i * 4), bar_value);
	}
}

void PCI_fill_func(uint8_t bus, uint8_t device, uint8_t header_type){
	if(header_type != PCI_DEVICE_HEADER_TYPE_GENERAL){
		return;
	}

	for(int func = 0; func < 8; func++){
		uint16_t vendor_id = PCI_controller_read_word(bus, device, func, 0);
		if(vendor_id == 0xFFFF){
			continue;
		}

		printf("-------------------------- %i\n", nb_pci_device);
		devices[nb_pci_device].vendor_id = vendor_id;
		devices[nb_pci_device].device_id = PCI_controller_read_word(bus, device, func, 0x2);
		devices[nb_pci_device].command = PCI_controller_read_word(bus, device, func, 0x4);
		devices[nb_pci_device].status = PCI_controller_read_word(bus, device, func, 0x6);
		devices[nb_pci_device].prog_if = PCI_controller_read_word(bus, device, func, 0x9) >> 8;
		devices[nb_pci_device].subclass = PCI_controller_read_word(bus, device, func, 0xA) & 0xFF;
		devices[nb_pci_device].class_code = PCI_controller_read_word(bus, device, func, 0xB) >> 8;
		devices[nb_pci_device].nb_bar = 6;
		print_device(&devices[nb_pci_device]);
		PCI_get_bar(bus, device, func, &devices[nb_pci_device]);
		nb_pci_device++;
	}
}

void PCI_scan(){
	printf("\nPCI device:\n");

	for(int bus = 0; bus < 256; bus++){
		for(int device = 0; device < 32; device++){
			uint16_t vendor_id = PCI_controller_read_word(bus, device, 0, 0);
			if(vendor_id == 0xFFFF){
				continue;
			}
			uint8_t header_type = PCI_controller_read_word(bus, device, 0, 0xE) & 0xFF;
			PCI_fill_func(bus, device, header_type);
		}
	}
}

void print_all_pci_devices(){
	for(int i = 0; i < nb_pci_device; i++){
		print_device(&devices[i]);
	}
}

uint16_t pci_get_device_by_class(uint8_t class, uint16_t* data_out){
	uint16_t size = 0;
	for(int i = 0; i < nb_pci_device; i++){
		if(devices[i].class_code == class){
			data_out[size] = i;
			size++;
		}
	}
	return size;
}

void pci_get_device_info(uint16_t pci_d, uint8_t* out){
	out[0] = devices[pci_d].class_code;
	out[1] = devices[pci_d].subclass;
	out[2] = devices[pci_d].prog_if;
}

pci_bar_t* pci_get_port_info(uint16_t pci_d, uint16_t* size){
	*size = devices[pci_d].nb_bar;
	return devices[pci_d].bar;
}
