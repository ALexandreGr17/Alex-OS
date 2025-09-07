#include "arch/i686/io.h"
#include <driver/acpi/acpi.h>
#include "./pci.h"
#include <stdint.h>
#include <stdio.h>

#define PCI_DATA_PORT		0xCFC
#define PCI_COMMAND_PORT	0xCF8

#define MAX_PCI_DEVICE 8 * 32 * 8

#define REG(x) x * sizeof(uint32_t)

// Read command
// 31	 -> Enable read
// 30-24 -> reserved
// 23-16 -> bus
// 15-11 -> device
// 10-8  -> function
// 7-2   -> offset
// 1-0   -> should be 0

PCI_device_t devices[8 * 32 * 8];
int nb_device = 0;

uint32_t PCI_controller_read(PCI_device_t* dev, uint8_t offset, uint8_t size) {
	uint32_t address;
	uint32_t lbus = dev->bus;
	uint32_t lslot = dev->slot;
	uint32_t lfunc = dev->func;

	address = (uint32_t)((lbus << 16) | 
			(lslot << 11) | 
			(lfunc << 8) | 
			(offset & 0xFC) | 
			((uint32_t)1 << 31));

	i686_outl(PCI_COMMAND_PORT, address);
	uint32_t val = i686_inl(PCI_DATA_PORT) >> (offset & 0x3) * 8;
	switch (size) {
		case BYTE:
			return (uint8_t)val;
		case WORD:
			return (uint16_t)val;
		case DWORD:
			return (uint32_t)val;
		default:
			return 0;
	}
}

void PCI_controller_write(PCI_device_t* dev, uint8_t offset, uint32_t value, uint8_t size){
	uint32_t address;
	uint32_t lbus = dev->bus;
	uint32_t lslot = dev->slot;
	uint32_t lfunc = dev->func;

	address = (uint32_t)((lbus << 16) | 
			(lslot << 11) | 
			(lfunc << 8) | 
			(offset & 0xFC) | 
			((uint32_t)1 << 31));

	i686_outl(PCI_COMMAND_PORT, address);
	uint32_t oldval = i686_inl(PCI_DATA_PORT);
	uint32_t mask;

	switch (size) {
		case BYTE:
			mask = 0xFF;
			break;
		case WORD:
			mask = 0xFFFF;
			break;
		case DWORD:
			mask = 0xFFFFFFFF;
			break;
	}
	int bitoffset = (offset & 3) * 8;
	value = (value & mask) << bitoffset;
	oldval &= ~(mask << bitoffset);
	oldval |= value;

	i686_outl(PCI_COMMAND_PORT, address);
	i686_outl(PCI_DATA_PORT, oldval);
}

void get_bar(PCI_device_t *dev) {
	for(int i = 0; i < 6; i++){
		uint8_t offset = 0x10 + i * sizeof(uint32_t); 
		uint32_t base_low = PCI_controller_read(dev, offset, DWORD);
		PCI_controller_write(dev, offset, ~0, DWORD);
		uint32_t size_low = PCI_controller_read(dev, offset, DWORD);
		PCI_controller_write(dev, offset, base_low, DWORD);

		if(base_low & 1){
			// IO mode
			dev->bars[i].type = BAR_TYPE_IO;
			dev->bars[i].addr = base_low & ~0b11;
			dev->bars[i].size = ~(size_low & ~0b11) + 1;
		}
		else{
			// MMIO mode
			int type = (base_low >> 1) & 3;
			uint32_t base_high = PCI_controller_read(dev, offset + 4, DWORD);
			dev->bars[i].addr = base_low & ~0xF;
			if (type == 2){
				// 64 bit
				dev->bars[i].addr |= ((uint64_t)base_high << 32);
				i++;
			}
			dev->bars[i].size = ~(size_low & 0b1111) + 1;
			dev->bars[i].type = BAR_TYPE_MMIO;
		}
	}
}

void print_device(PCI_device_t* dev){
	printf("PCI Device:\n");
	printf("bus: %i, slot: %i, func: %i\n", dev->bus, dev->slot, dev->func);
	printf("vendor_id: 0x%x, device_id: 0x%x\n", dev->vendor_id, dev->device_id);
	printf("class_code: 0x%x, subclass: 0x%x, prog_if: 0x%x\n", dev->class_code, dev->subclass, dev->prog_if);
	printf("int_pin: %i, int_line: %i\n", dev->int_pin, dev->int_line);
	printf("BARS:\n");
	for(int i = 0; i < 6; i++){
		if (dev->bars[i].addr != 0)
		{
			printf("\tbar %i: type %d, base 0x%lx\n", i, dev->bars[i].type, dev->bars[i].addr);
		}
	}
	printf("\n");
}

void scan_bus(uint8_t bus){
	for (int i = 0; i < 32; i++){
		for (int j = 0; j < 8; j++) {
			devices[nb_device].bus = bus;
			devices[nb_device].slot = i;
			devices[nb_device].func = j;

			if (PCI_controller_read(&devices[nb_device], 0, WORD) == 0xFFFF){
				continue;
			}

			if (PCI_controller_read(&devices[nb_device], 0xC, DWORD) & 0x800000){
				continue;
			}

			devices[nb_device].vendor_id = PCI_controller_read(&devices[nb_device], REG(0), WORD);
			devices[nb_device].device_id = PCI_controller_read(&devices[nb_device], REG(0), DWORD) >> 16;
			devices[nb_device].class_code = PCI_controller_read(&devices[nb_device], REG(2), DWORD) >> 24;
			devices[nb_device].subclass = PCI_controller_read(&devices[nb_device], REG(2), DWORD) >> 16;
			devices[nb_device].prog_if = PCI_controller_read(&devices[nb_device], REG(2), DWORD) >> 8;
			devices[nb_device].int_line = PCI_controller_read(&devices[nb_device], REG(0xF), DWORD);
			devices[nb_device].int_pin = PCI_controller_read(&devices[nb_device], REG(0xF), DWORD) >> 8;
			get_bar(&devices[nb_device]);
			//print_device(&devices[nb_device]);
			nb_device++;
		}
		
	}
}

void scan_root_bus(void) {
	PCI_device_t dev = { 0 };
	if (PCI_controller_read(&dev, 0xC, DWORD) & 0x800000){
		// if MF bit in header type set then only 1 bus
		scan_bus(0);
	}
	else {
		for (int i = 0; i < 8; i++) {
			scan_bus(i);
		}
	}
}


int PCI_get_device_by_filter(PCI_device_t *filter, char mask, int *id_out, int size){
	int c = 0;
	for (int i = 0; i < nb_device && c < size; i++) {
		if (mask & 1 && (devices[i].vendor_id != filter->vendor_id))
			continue;
		if (mask & 2 && (devices[i].device_id != filter->device_id))
			continue;
		if (mask & 4 && (devices[i].class_code != filter->class_code))
			continue;
		if (mask & 8 && (devices[i].subclass != filter->subclass))
			continue;
		if (mask & 16 && (devices[i].prog_if != filter->prog_if))
			continue;
		id_out[c] = i;
		c++;
	}
	return c;
}

PCI_device_t *PCI_get_device_by_id(int id){
	return &devices[id];
}

int PCI_get_nb_device(){
	return nb_device;
}

void PCI_init(){
	scan_root_bus();
}
