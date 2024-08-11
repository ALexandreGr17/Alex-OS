#include <arch/i686/ata.h>
#include <arch/i686/io.h>
#include <stdint.h>
#include <stdio.h>

void ata_init(disk_ata_t* ata, uint8_t master){
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
		printf("0x%x ", i686_inw(ata->base_port));
		ata_delay(ata);
	}
}

void ata_read28(disk_ata_t* ata, uint32_t lba, uint8_t* data, uint16_t count){
	if(lba & 0xF0000000){
		return;
	}
	
	i686_outb(ata->drive_select_port, (ata->master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
	ata_delay(ata);

	i686_outb(ata->sector_port, 1);
	i686_outb(ata->lba_lo_port, lba & 0xFF);
	i686_outb(ata->lba_mid_port, (lba >> 8) & 0xFF);
	i686_outb(ata->lba_hi_port, (lba >> 16) & 0xFF);

	i686_outb(ata->command_port, 0x20);

	int8_t status = i686_inb(ata->status_port);
	while(status & 0x80 && !(status & 1)){
		status = i686_inb(ata->status_port);
	}

	if(status & 1){
		printf("error while reading from disk\n");
		return;
	}

	for(int i = 0; i < count; i+=2){
		uint16_t data16 = i686_inw(ata->base_port);
		data[i] = (data16 >> 8) & 0xFF;
		if(i+1 < count){
			data[i+1] = data16 & 0xFF;
		}
	}

	for(int i = count; i < 512; i++){
		i686_inw(ata->base_port);
	}
}

void ata_write28(disk_ata_t* ata, uint32_t lba, uint8_t* data, uint16_t count){
	if(lba & 0xF0000000){
		return;
	}

	if(count > 512){
		return;
	}
	
	i686_outb(ata->drive_select_port, (ata->master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
	ata_delay(ata);

	i686_outb(ata->sector_port, 1);
	i686_outb(ata->lba_lo_port, lba & 0xFF);
	i686_outb(ata->lba_mid_port, (lba >> 8) & 0xFF);
	i686_outb(ata->lba_hi_port, (lba >> 16) & 0xFF);

	i686_outb(ata->command_port, 0x30);

	printf("\nWriting data\n");
	for(uint16_t i = 0; i < count; i+=2){
		uint16_t data16 = data[i] << 8;
		if (i+1 < count){
			data16 |= data[i+1];
		}
		i686_outw(ata->base_port, data16);
	}

	for(uint16_t i = count + (count % 2); i < 512; i+=2){
		uint16_t data16 = 0;
		i686_outw(ata->base_port, data16);
	}
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
