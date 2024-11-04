#ifndef ACPI_H
#define ACPI_H
#include <stdint.h>

typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		OEMID[6];
	uint8_t		OEMID_table[8];
	uint32_t	OEM_revision;
	uint32_t	creator_ID;
	uint32_t	creator_revision;
} __attribute__((packed)) SDT_hdr_t;

void acpi_init();
SDT_hdr_t* find_descriptor_table(char signature[4]);

#endif
