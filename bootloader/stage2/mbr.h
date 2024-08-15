#ifndef MBR_H
#define MBR_H

#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	DISK* disk;
	uint32_t partition_offset;
	uint32_t partition_size;
} partition_t;

void MBR_detect_partition(DISK* disk, void* address, partition_t* partition_out);
bool MBR_partition_read_sector(partition_t* disk, uint32_t lba, uint8_t sectors, void* data_out);
#endif
