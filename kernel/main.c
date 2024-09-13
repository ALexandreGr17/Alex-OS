#include "arch/i686/fdc.h"
#include "disk.h"
#include "errno.h"
#include "string/string.h"
#include <stdint.h>
#include <arch/i686/isr.h>
#include <boot/bootparams.h>
#include <stdio.h>
#include <memory/memory.h>
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/keyboard.h>
#include <arch/i686/pci/pci.h>
#include <arch/i686/ata.h>
#include <mem_management.h>
#include <filesystem/fat.h>

extern uint8_t __bss_start;
extern uint8_t __end;
void crash_me();

void timer(Register* regs){
	//printf(".");
}

void __attribute__((section(".entry"))) start(boot_parameters_t* bootparams){
	memset(&__bss_start, 0, (&__end) - (&__bss_start));
	clrscr();

	init_memory_management(&bootparams->Memory);
	HAL_Initialaize();
	
	for(int i = 0; i < 16; i++){
		if(i != 1 && i != 6){
			i686_IRQ_RegisterHandler(i, timer);
		}
	}

	printf("Hello world from kernel\n");
	printf("BootDevice: 0x%x\n", bootparams->BootDevice);
	printf("Memory region count: %i\n", bootparams->Memory.region_count);
	for(int i = 0; i < bootparams->Memory.region_count; i++){
		printf("Start: 0x%llx, length: 0x%llx, type: 0x%x\n", 
				bootparams->Memory.regions[i].Begin, 
				bootparams->Memory.regions[i].Length,
				bootparams->Memory.regions[i].Type);
	}

	

	print_all_pci_devices();
	uint16_t pci_ds[2048];
	uint16_t nb_found = pci_get_device_by_class(0x3, pci_ds);
	if(nb_found == 0){
		printf("NO DISK\n");
		goto end;
	}

	printf("location: 0x%lx\n", bootparams->partition_location);

	/*
	uint16_t size;
	pci_bar_t* bars = pci_get_port_info(pci_ds[0], &size);
	for(int i = 0; i < size; i++){
		printf("DISK: port: 0x%x, type: 0x%x\n", bars[i].addr._16, bars[i].type);
	}*/


	//crash_me();
	
	printf("\n\n");
	disk_ata_t atam0 = {.base_port = 0x1F0, .master = 1};
	ata_init(&atam0, 1, bootparams->partition_location);
	identify(&atam0);

	disk_t disk = {
		.disk = &atam0,
		.disk_read = &ata_read28,
		.disk_write = &ata_write28
	};

	if(!FAT_init(&disk)){
		printf("FAT init failed errno: 0x%x\n", errno);
		goto end;
	}

	printf("FAT init\n");
	/*int handle = FAT_open(&disk, "/test/test.txt");
	if(handle < 0){
		goto end;
	}
	char buffer[32] = {0};
	printf("%d\n", handle);
	int i = 0;
	if((i = FAT_read(&disk, handle, 31, buffer)) != 31){
		printf("ERROR ----------------------> %d\n", i);
	}
	printf("%s\n", buffer);*/

	FAT_create_file(&disk, "/test/azer.txt");
	printf("------------------------------------------\n");
	int handle = FAT_open(&disk, "/test/azer.txt");
	printf("%d\n", handle);
	char* test = "Yo ca fonctionne\n";
	/*FAT_write(&disk, handle, strlen(test), test);
	printf("Written\n");
	FAT_read(&disk, handle, strlen(test), test);
	printf("%s\n", test);*/

/*
	char* buffer = "Hello world";
	ata_write28(&atam0, 0, buffer, 11);
	ata_flush(&atam0);
	char buffer_read[12] = {0};
	ata_read28(&atam0, 0, buffer_read, 11);
	printf("\n%s\n", buffer_read);*/
	
end:
	for(;;);
}
