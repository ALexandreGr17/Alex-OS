#ifndef BOOTPARAMS_H
#define BOOTPARAMS_H

#include <stdint.h>

typedef struct {
	uint64_t Begin;
	uint64_t Length;
	uint32_t Type;
	uint32_t ACPI;
} memory_region_t;

typedef struct {
	int region_count;
	memory_region_t* regions;
}memory_info_t;

typedef struct {
	memory_info_t Memory;
	uint8_t BootDevice;
} boot_parameters_t;

#endif
