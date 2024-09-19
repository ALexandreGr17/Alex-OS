#include "disk.h"
#include "errno.h"
#include "stdio.h"
#include "string/string.h"
#include <stdint.h>
#include <vfs/vfs.h>
#include <filesystem/fat.h>
#include <mem_management.h>
#include <vfs/std.h>
#include <errno.h>

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

int open(char* path, uint8_t create){
	char* tmp_path = calloc(strlen(path) + 1, 0);
	strcpy(tmp_path, path);

	int handle = FAT_open(disks[0], tmp_path);
	if(handle < 0 && errno == 0xff && create){
		strcpy(tmp_path, path);
		if(!FAT_create_file(disks[0], tmp_path)){
			printf("aie\n");
		}
		strcpy(tmp_path, path);
		handle = FAT_open(disks[0], tmp_path);
	}
	free(tmp_path);
	return handle;
}

void read_line(int handle, uint32_t* size, void** out){
	if(handle >= 3){
		FAT_read(disks[0], handle, *size, *out);
		return;
	}

	*size = 100;
	char* buffer = calloc(*size, 0);
	char c;
	uint32_t i = 0;
	while(c != '\n'){
		c = std_get(stds[handle]);
		if(c != 0){
			if(i >= *size){
				*size += 100;
				buffer = realloc(buffer, *size);
			}
			buffer[i] = c;
			i++;
		}
	}

	*out = buffer;
	*size = i;
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

void seek(int handle, uint32_t offset, uint8_t where){
	FAT_seek(disks[0], handle, offset, where);
}

uint32_t tellpos(int handle){
	return FAT_tellpos(handle);
}

void list(int handle){
	FAT_list(disks[0], handle);
}
