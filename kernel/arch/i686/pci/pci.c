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
	PCI_DEVICE_HEADER_TYPE_PCI_2_PCI_BRIDGE = 1,
	PCI_DEVICE_HEADER_TYPE_PCI_2_CARDBUS_BRIDGE = 2,
} PCI_DEVICE_HEADER_TYPE;

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

void print_device(hardware_pci_device_t* device){
	printf("PCI Device: \nvendor_id: 0x%x, device_id: 0x%x, class: 0x%x, subclass: 0x%x, prog_if: 0x%x\n",
			device->vendor_id, device->device_id, device->class_code, device->subclass, device->prog_if);
}

uint8_t get_highest_bit_set(uint32_t val){
	uint8_t pos = 31;
	uint32_t mask = 1 << pos;
	while(!(val & mask)){
		pos--;
		mask >>= 1;
	}
	return pos;
}

uint8_t get_lowest_bit_set(uint32_t val){
	uint8_t pos = 0;
	uint32_t mask = 1;
	while(!(val & mask)){
		pos++;
		mask <<= 1;
	}
	return pos;
}


void get_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t header_type, hardware_pci_device_t* device){
	int max_bar = (header_type == PCI_DEVICE_HEADER_TYPE_GENERAL) ? 6 : 2;
	device->nb_bar = max_bar;
	for(int i = 0; i < max_bar; i++){
		uint32_t bar_value = pci_controller_read(bus, slot, func, 0x10 + 4 * i);
		if(device->class_code == 0x6){
			//printf("0x%x\n", bar_value);
		}
		if((bar_value & 1) == BAR_TYPE_MEM){
			device->bar[i].type = BAR_TYPE_MEM;
			device->bar[i].prefetchable = (bar_value & 0x4) >> 2;
			switch (bar_value & (3 << 1)){
				case BAR_SIZE_16:
					device->bar[i].size = BAR_SIZE_16;
					break;

				case BAR_SIZE_32:
					device->bar[i].size = BAR_SIZE_32;
					pci_controller_write(bus, slot, func, 0x10 + 4 * i, 0xFFFFFFF0);
					uint32_t new_bar_value = pci_controller_read(bus, slot, func, 0x10 + 4 * i) & 0xFFFFFFF0;
					if(new_bar_value == 0){
						break;
					}

					uint8_t lowest_bit = get_lowest_bit_set(new_bar_value);
					uint8_t highest_bit = get_highest_bit_set(new_bar_value);

					if(((highest_bit != 31) && (highest_bit != 15 )) || (highest_bit < lowest_bit) || (lowest_bit < 2)){
						break;
					}

					uint8_t nb_byte_addr = highest_bit + 1;
					uint32_t size_addr = 1U << lowest_bit;
					pci_controller_write(bus, slot, func, 0x10 + 4 * i, bar_value);
					break;

				case BAR_SIZE_64:
					device->bar[i].size = BAR_SIZE_64;
					device->bar[i].addr._64 = bar_value & 0xFFFFFFF0;
					uint32_t next_value = pci_controller_read(bus, slot, func, 0x10 + 4 * (i+1));
					i++;
					break;
			}
		}
		else {
			device->bar[i].type = BAR_TYPE_IO;
			device->bar[i].prefetchable = 0;
			device->bar[i].addr._16 = bar_value & 0xFFFC;
			device->bar[i].size = BAR_SIZE_16;
		}
	}
}

void pci_scan(){
	//printf("\nPCI device:\n", PCI_COMMAND_PORT);

	for(int bus = 0; bus < 8; bus++){
		for(int device = 0; device < 32; device++){
			int nb_func = device_has_multiple_function(bus, device) ? 8 : 1;
			for(int func = 0; func < nb_func; func++){
				uint16_t vendor_id = pci_controller_read(bus, device, func, 0);
				uint16_t header_type = pci_controller_read(bus, device, func, 0xE);
				if(vendor_id == 0xFFFF || header_type == PCI_DEVICE_HEADER_TYPE_PCI_2_CARDBUS_BRIDGE){
					continue;
				}

				devices[nb_pci_device].vendor_id = vendor_id;
				devices[nb_pci_device].device_id = pci_controller_read(bus, device, func, 0x2);
				devices[nb_pci_device].command = pci_controller_read(bus, device, func, 0x4);
				devices[nb_pci_device].status = pci_controller_read(bus, device, func, 0x6);
				devices[nb_pci_device].prog_if = pci_controller_read(bus, device, func, 0x9) >> 8;
				devices[nb_pci_device].subclass = pci_controller_read(bus, device, func, 0xA);
				devices[nb_pci_device].class_code = pci_controller_read(bus, device, func, 0xB) >> 8;
				get_bar(bus, device, func, header_type, &devices[nb_pci_device]);
				nb_pci_device++;
			}
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

pci_bar_t* pci_get_port_info(uint16_t pci_d, uint16_t* size){
	*size = devices[pci_d].nb_bar;
	return devices[pci_d].bar;
}
