#ifndef RSDP_H
#define RSDP_H

#include <stdint.h>
typedef struct {
	char		signature[8];
	uint8_t		checksum;
	char		OEMID[6];
	uint8_t		revision;
	uint32_t    RSDT_address; // only for version 1.0

	uint32_t    Length;
	uint64_t	XSDT_address;
	uint8_t		extended_cheksum;
} __attribute__((packed)) RSDP_t;

void find_RSDP(RSDP_t** rsdp);

#endif
