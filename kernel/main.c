#include "arch/i686/fdc.h"
#include "disk.h"
#include "errno.h"
#include <memory_management/virtual/virtual_memory_manager.h>
#include <memory_management/physique/physical_memory_manager.h>
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
#include <filesystem/fat.h>
#include "vfs/vfs.h"
#include <arch/i686/acpi.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void crash_me();

char* memory_reg_type(uint8_t type) {
    switch (type) {
        case 1:
            return "Available";
        case 2:
            return "Reserved";
        case 3:
            return "ACPI Reclaimed";
        case 4:
            return "ACPI NVS";
        default:
            return "Reserved";
    
    }
}

void __attribute__((section(".entry"))) start(boot_parameters_t* bootparams){
    memset(&__bss_start, 0, (&__end) - (&__bss_start));
	clrscr();

    printf("Welcom to AlexOS\n");
	printf("Memory region count: %i\n", bootparams->Memory.region_count);
	for(int i = 0; i < bootparams->Memory.region_count; i++){
		printf("Start: 0x%llx, length: 0x%llx, type: %s\n", 
				bootparams->Memory.regions[i].Begin, 
				bootparams->Memory.regions[i].Length,
				memory_reg_type(bootparams->Memory.regions[i].Type));
	}

	HAL_Initialaize();

    memory_region_t* last = &bootparams->Memory.regions[bootparams->Memory.region_count - 1];
    uint32_t total_memory = last->Begin + last->Length - 1;
    init_physical_memory_manager(0x30000, total_memory);

    // init memory region fot the Available memory region
    for (uint32_t i = 0; i < bootparams->Memory.region_count; i++) {
        if (bootparams->Memory.regions[i].Type == 1) {
            init_physical_memory_region(bootparams->Memory.regions[i].Begin, bootparams->Memory.regions[i].Length);
        }
    }

    // Set certain regions/blocks as used or reserved
    delete_physical_memory_region(0x1000, 0x9000);

    print_physical_mem_info();

    printf("Initialize paging\n");
    int i = init_virtual_memory_manager(bootparams->kernel_location);
    printf("%d\n", i);
    
    // Identity  map
    
//

//
//	printf("Hello world from kernel\n");
//	printf("BootDevice: 0x%x\n", bootparams->BootDevice);
//	printf("location: 0x%lx\n", bootparams->partition_location);
//	printf("\n\n");
//
//	disk_ata_t atam0 = {.base_port = 0x1F0, .master = 1};
//	ata_init(&atam0, 1, bootparams->partition_location);
//	identify(&atam0);
//
//	disk_t disk = {
//		.disk = &atam0,
//		.disk_read = &ata_read28,
//		.disk_write = &ata_write28
//	};
//
//	disk_t* disks = &disk;
//	vfs_init(&disks, 1);
//
//	if(!FAT_init(&disk)){
//		printf("FAT init failed errno: 0x%x\n", errno);
//		goto end;
//	}
//
//	printf("FAT init\n");
//
//	FAT_create_file(&disk, "/test/azer.txt");
//	printf("------------------------------------------\n");
//
//	int handle = FAT_open(&disk, "test/azer.txt");
//
//	char* test = "Yo ca fonctionne\n";
//	FAT_write(&disk, handle, strlen(test), (uint8_t*)test);
//	FAT_seek(&disk, handle, 0, SEEK_SET);
//	FAT_read(&disk, handle, strlen(test), (uint8_t*)test);
//	close(handle);
//
//
//	char* buffer = "Hello world";
//	ata_write28(&atam0, 0, buffer, 11);
//	ata_flush(&atam0);
//	char buffer_read[12] = {0};
//	ata_read28(&atam0, 0, buffer_read, 11);
//	printf("\n%s\n", buffer_read);
//	
//	
end:
	for(;;);
}

