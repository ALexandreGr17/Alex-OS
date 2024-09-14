#include "disk.h"
#include "stdio.h"
#include <stdint.h>
#include <vfs/vfs.h>
#include <filesystem/fat.h>
#include <mem_management.h>
#include <vfs/std.h>

disk_t** disks;
std_t* stds[3];

void vfs_init(disk_t** disk, uint8_t nb_disk){
	disks = malloc(sizeof(disk_t*) * nb_disk);
	for(int i = 0; i < nb_disk; i++){
		disks[i] = disk[i];
	}

	for(int i = 0; i < 3; i++){
		stds[i] = init_std();
	}
}

int open(char* path){
	return FAT_open(disks[0], path);
}

uint32_t read(int handle, uint32_t size, void* out){
	if(handle >= 3){
		return FAT_read(disks[0], handle, size, out);
	}

	char c = 1;
	uint32_t count = 0;
	while (c != '\n') {
		c = std_get(stds[handle]);
		if(count < size && c != 0){
			*(char*)out = c;
			out++;
			count++;
		}
	}
	return count;
}

uint32_t write(int handle, uint32_t size, void* in){
	if(handle >= 3){
		return FAT_write(disks[0], handle, size, in);
	}

	for(int i = 0; i < size; i++){
		std_put(stds[handle], *(char*)in);
		in++;
	}
	return size;
}

void close(int handle){
	if(handle >= 3){
		FAT_close(handle);
	}
}

