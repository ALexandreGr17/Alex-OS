#ifndef DISK_H
#define DISK_H

#include <stdint.h>
typedef struct {
	void* disk;
	uint8_t (*disk_read)(void* disk, uint32_t lba, uint8_t sector_count, uint8_t* data_out);
	uint8_t (*disk_write)(void* disk, uint32_t lba, uint8_t sector_count, uint8_t* data_in);
} disk_t;

#endif
