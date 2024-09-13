#include <arch/i686/ata.h>
#include <arch/i686/io.h>
#include <stdint.h>
#include <stdio.h>

void ata_init(disk_ata_t* ata, uint8_t master, uint32_t partition_offset){
	ata->error_port = ata->base_port + 1;
	ata->sector_port = ata->base_port + 2;
	ata->lba_lo_port = ata->base_port + 3;
	ata->lba_mid_port = ata->base_port + 4;
	ata->lba_hi_port = ata->base_port + 5;
	ata->drive_select_port = ata->base_port + 6;
	ata->status_port = ata->base_port + 7;
	ata->command_port = ata->base_port + 7;
	
	ata->control_port = ata->base_port + 0x206;

	ata->master = master;
	ata->partition_offset = partition_offset;
}

void ata_delay(disk_ata_t* ata){
	for(int i = 0; i < 15; i++){
		i686_inb(ata->status_port);
	}
}

void identify(disk_ata_t* ata){
	// seting up for the identify cmd
	i686_outb(ata->drive_select_port, ata->master ? 0xA0 : 0xB0);
	ata_delay(ata);
	//i686_outb(ata->control_port, 0);

	uint8_t status = i686_inb(ata->status_port);
	if(status == 0xFF){
		printf("Pas de sique detecte\n");
		return;
	}

	i686_outb(ata->sector_port, 0);
	i686_outb(ata->lba_lo_port, 0);
	i686_outb(ata->lba_mid_port, 0);
	i686_outb(ata->lba_hi_port, 0);
	ata_delay(ata);
	// sending IDENTIFY cmd
	i686_outb(ata->command_port, 0xEC);
	ata_delay(ata);
	status = i686_inb(ata->status_port);
	if(status == 0){
		printf("Pas de peripherique ATA\n");
		return;
	}

	while(status & 0x80 && !(status & 1)){
		status = i686_inb(ata->status_port);
		i686_iowait();
	}

	if(status & 1){
		printf("Error 1\n");
		return;
	}

	if(i686_inb(ata->lba_mid_port) || i686_inb(ata->lba_hi_port)){
		printf("Error 2\n");
		return;
	}


	for(int i = 0; i < 256; i++){
		i686_inw(ata->base_port);
	//	printf("0x%x ", i686_inw(ata->base_port));
		ata_delay(ata);
	}
}

#define SECTOR_SIZE 512

uint8_t ata_read28(void* disk, uint32_t lba, uint8_t nb_sectors, uint8_t* data){
	disk_ata_t* ata = disk;
	if(lba & 0xF0000000){
		return -1;
	}

	uint32_t lba_real = ata->partition_offset + lba;


	i686_outb(ata->drive_select_port, (ata->master ? 0xE0 : 0xF0) | ((lba_real >> 24) & 0x0F));
	ata_delay(ata);

	i686_outb(ata->sector_port, nb_sectors);
	i686_outb(ata->lba_lo_port, lba_real & 0xFF);
	i686_outb(ata->lba_mid_port, (lba_real >> 8) & 0xFF);
	i686_outb(ata->lba_hi_port, (lba_real >> 16) & 0xFF);

	i686_outb(ata->command_port, 0x20);

	for(int i = 0; i < nb_sectors; i++){
		int8_t status = i686_inb(ata->status_port);
		while(status & 0x80 && !(status & 1)){
			status = i686_inb(ata->status_port);
		}

		if(status & 1){
			printf("error while reading from disk\n");
			return -2;
		}

		for(int j = 0; j < SECTOR_SIZE; j+=2){
			uint16_t data16 = i686_inw(ata->base_port);
			data[SECTOR_SIZE * i + j] = data16 & 0xFF;
			if(j+1 < SECTOR_SIZE){
				data[SECTOR_SIZE * i + j+1] = (data16 >> 8) & 0xFF;
			}
		}
	}
	return 0;
}

uint8_t ata_write28(void* disk, uint32_t lba, uint8_t nb_sectors,uint8_t* data){
	disk_ata_t* ata = disk;
	if(lba & 0xF0000000){
		return -1;
	}

	uint32_t lba_real = ata->partition_offset + lba;

	i686_outb(ata->drive_select_port, (ata->master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
	ata_delay(ata);

	i686_outb(ata->sector_port, nb_sectors);
	i686_outb(ata->lba_lo_port, lba_real & 0xFF);
	i686_outb(ata->lba_mid_port, (lba_real >> 8) & 0xFF);
	i686_outb(ata->lba_hi_port, (lba_real >> 16) & 0xFF);

	i686_outb(ata->command_port, 0x30);

	//printf("\nWriting data\n");
	for(int i = 0; i < nb_sectors; i++){
		for(uint16_t j = 0; j < SECTOR_SIZE; j+=2){
			uint16_t data16 = data[i * SECTOR_SIZE + j+1] << 8;
			if (i+1 < SECTOR_SIZE){
				data16 |= data[i * SECTOR_SIZE + j];
			}
			i686_outw(ata->base_port, data16);
		}
	}
	return 0;
}

void ata_flush(disk_ata_t* ata){
	i686_outb(ata->drive_select_port, (ata->master ? 0xE0 : 0xF0));
	ata_delay(ata);
	i686_outb(ata->command_port, 0xE7);

	int8_t status = i686_inb(ata->status_port);
	while(status & 0x80 && !(status & 1)){
		status = i686_inb(ata->status_port);
	}

	if(status & 1){
		printf("Error while flushing\n");
	}
}
