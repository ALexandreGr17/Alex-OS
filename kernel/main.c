#include "ELF/ELF.h"
#include "arch/i686/fdc.h"
#include "disk.h"
#include "errno.h"
#include <memory_management/virtual/virtual_memory_manager.h>
#include <memory_management/physique/physical_memory_manager.h>
#include "memory_management/memory_management.h"
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

void term(disk_t* disk);

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

	HAL_Initialaize(bootparams);


    printf("Hello world from kernel\n");
    printf("BootDevice: 0x%x\n", bootparams->BootDevice);
    printf("partition location: 0x%lx\n", bootparams->partition_location);
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

    uint8_t* test;
    load_elf_file("bin/test.elf", &test);
    printf("%x\n", test[0]);
    printf("%x\n", test[1]);
    clrscr();
    ((void (*)())test)();
    term(disks);


end:
	for(;;);
}


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
		read_line(STDIN, &size, (void**)&buffer);
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
            if (size == 0) {
                printf("file is empty\n");
                continue;
            }
			seek(handle, 0, SEEK_SET);
			char* test = calloc(size, 1);
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
