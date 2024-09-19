#include "arch/i686/fdc.h"
#include "disk.h"
#include "errno.h"
#include "mem_management/heap.h"
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
#include "vfs/vfs.h"

extern uint8_t __bss_start;
extern uint8_t __end;
void crash_me();

void timer(Register* regs){
	//printf(".");
}


void term();

void __attribute__((section(".entry"))) start(boot_parameters_t* bootparams){
	memset(&__bss_start, 0, (&__end) - (&__bss_start));
	clrscr();

	printf("Memory region count: %i\n", bootparams->Memory.region_count);
	for(int i = 0; i < bootparams->Memory.region_count; i++){
		printf("Start: 0x%llx, length: 0x%llx, type: 0x%x\n", 
				bootparams->Memory.regions[i].Begin, 
				bootparams->Memory.regions[i].Length,
				bootparams->Memory.regions[i].Type);
	}


	init_memory_management(&bootparams->Memory);
	HAL_Initialaize();
	debug_heap();
	
	for(int i = 0; i < 16; i++){
		if(i != 1 && i != 6){
			i686_IRQ_RegisterHandler(i, timer);
		}
	}

	printf("Hello world from kernel\n");
	printf("BootDevice: 0x%x\n", bootparams->BootDevice);
		

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

	disk_t* disks = &disk;
	vfs_init(&disks, 1);

	if(!FAT_init(&disk)){
		printf("FAT init failed errno: 0x%x\n", errno);
		goto end;
	}

	printf("FAT init\n");

	FAT_create_file(&disk, "/test/azer.txt");
	printf("------------------------------------------\n");

	int handle = FAT_open(&disk, "test/azer.txt");

	printf("%d\n", handle);
	char* test = "Yo ca fonctionne\n";
	FAT_write(&disk, handle, strlen(test), test);
	printf("Written\n");
	FAT_seek(&disk, handle, 0, SEEK_SET);
	FAT_read(&disk, handle, strlen(test), test);
	printf("%s\n", test);
	close(handle);

/*
	char* buffer = "Hello world";
	ata_write28(&atam0, 0, buffer, 11);
	ata_flush(&atam0);
	char buffer_read[12] = {0};
	ata_read28(&atam0, 0, buffer_read, 11);
	printf("\n%s\n", buffer_read);*/
	
	term();
	
end:
	for(;;);
}

// TODO:
//		parse line
//		cat
//		ls
//		touch
//		mkdir

char* builtin[] = {
	"help",
	"cat",
	"quit",
	"test",
	"ls",
	"touch",
	"mkdir"
};

void exec_cmdline(char* line){
	char* old = line;
	for(int i = 0; line[i] && line[i] != '\n'; i++){
		if(line[i] == ' '){
			line[i] = 0;

		}
	}
}

void trim(char* buffer){
	for(int i = 0; buffer[i]; i++){
		if(buffer[i] == '\n'){
			buffer[i] = 0;
			return;
		}
	}
}

void term(disk_t* disk){
	for(;;){
		printf("> ");
		char* buffer = NULL;
		
		uint32_t size = 0;
		read_line(STDIN, &size, &buffer);
		trim(buffer);
		char* args = strchr(buffer, ' ');
		buffer[args - buffer] = 0;
		args++;
		//printf("you wrote: %s\n", buffer);
		if(strcmp(buffer, "quit")){
			printf("bye\n");
			return;
		}

		if(strcmp(buffer, "ls")){
			int handle = open(args, 0);
			list(handle);
		}

		if(strcmp(buffer, "cat")){
			int handle = open(args, 0);
			if(handle == -1){
				printf("aie\n");
				continue;
			}
			seek(handle, 0, SEEK_END);
			uint32_t size = tellpos(handle);
			seek(handle, 0, SEEK_SET);
			char* test = calloc(size, 0);
			read(handle, size, test);
			printf("%s", test);
			free(test);
			close(handle);
		}

		if(strcmp(buffer, "touch")){
			int handle = open(args, 1);
			if(handle < 0){
				printf("Failed\n");
				continue;
			}
			printf("new file %s created \n", args);
		}

		if(strcmp(buffer, "write")){
			char* filename = args;
			char* data = strchr(args, ' ');
			filename[data - filename] = 0;
			data++;
			int handle = open(filename, 0);
			if(handle < 0){
				printf("no such file %s\n", filename);
				continue;
			}
			write(handle, strlen(data), data);
		}

		if(strcmp(buffer, "help")){
			printf("Voici les commandes possible:\n\tls <path>: list les fichier et dossier dans le dossier <path>\n\tcat <file>: lis le fichier path\n\ttouch <file>: creer le fichier file\n\twrite <file> <data>: ecris data dans le fichier file\n\tclear: clear l'ecran\n");
		}

		if(strcmp(buffer, "clear")){
			clrscr();
		}

		free(buffer);
	}
}

