#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t id;
	uint16_t cylinders;
	uint16_t sectors;
	uint16_t heads;
}DISK;

bool DISK_Initialisation(DISK* disk, uint8_t drive_number);
bool Disk_ReadSector(DISK* disk, uint32_t lba, uint8_t nb_sectors, void* output_buffer);

#endif
