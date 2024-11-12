#ifndef PCI_H
#define PCI_H
#include <stdint.h>

enum BAR_TYPE {
	BAR_TYPE_IO,
	BAR_TYPE_MMIO,
};

enum BAR_SIZE {
	BAR_SIZE_16,
	BAR_SIZE_32,
	BAR_SIZE_64,
};

enum CLASS_CODE {
	UNCLASSIFIED					= 0x00,
	MASS_STORAGE					= 0x01,
	NETWORK_CONTROLLER				= 0x02,
	DISPLAY_CONTROLLER				= 0x03,
	MULTIMEDIA_CONTROLLER			= 0x04,
	MEMORY_CONTROLLER				= 0x05,
	BRIDGE							= 0x06,
	SIMPLE_COMM_CONTROLLER			= 0x07,
	BASE_SYSTEM_PRIPHERAL			= 0x08,
	INPUT_DEVICE_CONTOLLER			= 0x09,
	DOCKING_STATION					= 0xA,
	PROCESSOR						= 0xB,
	SERIAL_BUS_CONTROLLER			= 0xC,
	WIRELESS_CONTROLLER				= 0xD,
	INTELLIGENT_CONTROLLER			= 0xE,
	SATELLITE_COMM_CONTROLLER		= 0xF,
	ENCRYPTION_CONTROLLER			= 0x10,
	SIGNAL_PROCESSING_CONTROLLER	= 0x11,
};

enum FILTER_MASK {
	VENDOR_ID = 1,
	DEVICE_ID = 2,
	CLASS_CODE = 4,
	SUBCLASS = 8,
	PROG_IF = 16,
};

typedef struct {
	enum BAR_TYPE type;
	enum BAR_SIZE size;
	uint64_t addr;
} PCI_bar_t;

typedef struct {
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
	uint8_t int_pin;
	uint8_t int_line;
	PCI_bar_t bars[6];

	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t  class_code;
	uint8_t subclass;
	uint8_t prog_if;
} PCI_device_t;

void PCI_init();
int PCI_get_nb_device();
int PCI_get_device_by_filter(PCI_device_t *filter, char mask, int *id_out, int size);
PCI_device_t *PCI_get_device_by_id(int id);
#endif
