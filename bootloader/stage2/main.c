#include <stdint.h>
#include "fat.h"
#include "memdefs.h"
#include "stdio.h"
#include "disk.h"
uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)();

void __attribute__((cdecl)) cstart(uint16_t bootDrive){
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

	if(!FAT_Initialize(&disk)){
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
	FAT_file* fd = FAT_Open(&disk, "./kernel.bin");
	if(!fd){
		putc('[');
		putc_color('x', 0x04);
		putc(']');
		printf(" Fat can't find kernel failed\r\n");
		goto end;
	}
	while((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer))){
		memcpy(kernel_buffer, KernelLoadBuffer, read);
		kernel_buffer += read;
	}
	FAT_Close(fd);
	
	KernelStart kernel_start = (KernelStart)kernel;
	kernel_start();

end:
	for(;;);
}