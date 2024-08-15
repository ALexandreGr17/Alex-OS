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

	uint8_t* kernel_buffer = kernel;
	uint32_t read;
	FAT_file* fd = FAT_Open(&partition_info, "./kernel.bin");
	if(!fd){
		putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Fat can't find kernel failed\r\n");
		goto end;
	}
	while((read = FAT_Read(&partition_info, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer))){
		memcpy(kernel_buffer, KernelLoadBuffer, read);
		kernel_buffer += read;
	}
	FAT_Close(fd);
	
	// prepare boot params
	
	Memory_detect(&bootparams.Memory);
	bootparams.BootDevice = bootDrive;

	KernelStart kernel_start = (KernelStart)kernel;
	kernel_start(&bootparams);

end:
	for(;;);
}
