#include <stdint.h>
#include "boot/bootparams.h"
#include "fat.h"
#include "memdefs.h"
#include "stdio.h"
#include "disk.h"
#include "memdetect.h"
#include "memory.h"
#include "mbr.h"
#include "elf.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
boot_parameters_t bootparams;

typedef void (*KernelStart)(boot_parameters_t* BootParams);

void __attribute__((cdecl)) cstart(uint16_t bootDrive, uint32_t partition){
	clrscr();

	DISK disk;
	if(!DISK_Initialisation(&disk, bootDrive)){
		putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Disk init failed \r\n");
		goto end;
	}
	putc('[');
	putc_color('x', 0x0a);
	putc(']');
	printf(" Disk init success\r\n");

	partition_t partition_info = {0};
	MBR_detect_partition(&disk, (void*)partition, &partition_info);
	if(!FAT_Initialize(&partition_info)){
		putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Fat init failed\r\n");
		goto end;
	}
	putc('[');
	putc_color('x', 0x0a);
	putc(']');
	printf(" FAT init success\r\n");


	Memory_detect(&bootparams.Memory);
	uint8_t* kernel_buffer = (uint8_t*)bootparams.Memory.regions[3].Begin;
	uint32_t read;

    KernelStart kernel_entry;
    printf("0x%x\n", partition_info.partition_offset);
    if (!ELF_read(&partition_info, "./kernel.elf", (void**)&kernel_entry)) {
        putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Kernel Load failed\r\n");
		goto end;
    }

    putc('[');
	putc_color('x', 0x0a);
	putc(']');
	printf(" Kernel Load success\r\n");

    bootparams.BootDevice = bootDrive;
	bootparams.partition_location = partition_info.partition_offset;
    bootparams.kernel_location = (uint32_t)kernel_entry;
    kernel_entry(&bootparams);

end:
	for(;;);
}
