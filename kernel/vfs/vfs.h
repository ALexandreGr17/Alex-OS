#ifndef VFS_H
#define VFS_H

#include "disk.h"
#include <stdint.h>

enum {
	STDIN = 0,
	STDOUT = 1,
	STDERR = 2,
};


void vfs_init(disk_t** disk, uint8_t nb_disk);

int open(char* path);
uint32_t read(int handle, uint32_t size, void* out);
uint32_t write(int handle, uint32_t size, void* in);
void close(int handle);
#endif
