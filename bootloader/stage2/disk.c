#include "disk.h"
#include "x86.h"
#include <stdbool.h>
#include <stdint.h>

bool DISK_Initialisation(DISK* disk, uint8_t drive_number){
	uint8_t drive_type;
	uint16_t sectors, cylinders, heads;

	if(!x86_Disk_GetDriveParams(drive_number, &drive_type, &cylinders, &sectors, &heads)){
		return false;
	}

	disk->cylinders = cylinders;
	disk->heads = heads;
	disk->sectors = sectors;
	disk->id = drive_number;
	return true;
}

void lba_to_chs(DISK* disk, uint32_t lba, uint16_t* c, uint16_t* h, uint16_t* s){
	*c = lba / (disk->sectors * disk->heads);
	*h = lba / disk->sectors % disk->heads;
	*s = lba % disk->sectors + 1;
}

bool Disk_ReadSector(DISK* disk, uint32_t lba, uint8_t nb_sectors, void* output_buffer){
	uint16_t c, h, s;
	lba_to_chs(disk, lba, &c, &h, &s);

	for(int i = 0; i < 3; i++){
		if(x86_Disk_Read(disk->id, c, h, s, nb_sectors, output_buffer)){
			return true;
		}
		if(!x86_Disk_Reset(disk->id)){
			return false;
		}
	}
	return false;
}
