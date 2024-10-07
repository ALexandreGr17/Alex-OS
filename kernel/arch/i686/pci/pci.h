#ifndef PCI_H
#define PCI_H
#include <stdint.h>

typedef struct {
	union {
		uint16_t _16;
		uint32_t _32;
		uint64_t _64;
	}addr;
	uint8_t type;
	uint8_t prefetchable;
	uint8_t size;
} pci_bar_t;

typedef struct {
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	command;
	uint16_t	status;
	uint8_t		prog_if;
	uint8_t		subclass;
	uint8_t		class_code;
	uint8_t		nb_bar;
	pci_bar_t   bar[6];
} hardware_pci_device_t;



void PCI_scan();
void print_all_pci_devices();
uint16_t pci_get_device_by_class(uint8_t class, uint16_t* data_out);
pci_bar_t* pci_get_port_info(uint16_t pci_d, uint16_t* size);
void pci_get_device_info(uint16_t pci_d, uint8_t* out);

#endif
