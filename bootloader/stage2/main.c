#include <stdint.h>
#include "boot/bootparams.h"
#include "fat.h"
#include "memdefs.h"
#include "stdio.h"
#include "disk.h"
#include "memdetect.h"
#include "memory.h"
#include "mbr.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* kernel = (uint8_t*)MEMORY_KERNEL_ADDR;
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
	FAT_file* fd = FAT_Open(&partition_info, "./kernel.bin");
	if(!fd){
		putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Fat can't find kernel failed\r\n");
		goto end;
	}

	uint64_t size = 0;
	while((read = FAT_Read(&partition_info, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer))){
		memcpy(kernel_buffer, KernelLoadBuffer, read);
		kernel_buffer += read;
		size += read;
	}
	size += (256 * 8) + 6 + 0x200; // sizeof IDT + tmp padding for malloc idk
	//printf("size 0x%x\n", size);
	bootparams.Memory.regions[3].Begin += size % 8 == 0 ? size : (size + (8 - size % 8));
	FAT_Close(fd);
	
	// prepare boot params
	
	bootparams.BootDevice = bootDrive;
	bootparams.partition_location = partition_info.partition_offset;
	KernelStart kernel_start = (KernelStart)kernel;
	kernel_start(&bootparams);

end:
	for(;;);
}
