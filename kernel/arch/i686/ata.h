#ifndef ATA_H
#define ATA_H
#include <stdint.h>
typedef struct {
	uint8_t		master;
	uint16_t	base_port;
	uint16_t	error_port;
	uint16_t	sector_port;
	uint16_t	lba_lo_port;
	uint16_t	lba_mid_port;
	uint16_t	lba_hi_port;
	uint16_t	drive_select_port;
	uint16_t	status_port;
	uint16_t	command_port;
	uint16_t	control_port;

	uint8_t		drive_number;
} disk_ata_t;

void ata_init(disk_ata_t* ata, uint8_t master);
void identify(disk_ata_t* ata);
void ata_read28(disk_ata_t* ata, uint32_t lba, uint8_t* data, uint16_t count);
void ata_write28(disk_ata_t* ata, uint32_t lba, uint8_t* data, uint16_t count);
void ata_flush(disk_ata_t* ata);

#endif
