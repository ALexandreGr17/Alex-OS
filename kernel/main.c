#include "arch/i686/fdc.h"
#include "disk.h"
#include "errno.h"
#include "mem_management/heap.h"
#include "mem_management/physique/physical_memory_manager.h"
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
#include <arch/i686/acpi.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void crash_me();

void timer(Register* regs){
	//printf(".");
}


void term();

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

//	debug_heap();
//
//	for(int i = 0; i < 16; i++){
//		if(i != 1 && i != 6){
//			i686_IRQ_RegisterHandler(i, timer);
//		}
//	}
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
//	debug_heap();
//	term();
//	
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
		if(strcmp(buffer, "ping")){
			printf("pong\n");
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

