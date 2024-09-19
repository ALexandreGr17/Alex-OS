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

int open(char* path, uint8_t create);
uint32_t read(int handle, uint32_t size, void* out);
uint32_t write(int handle, uint32_t size, void* in);
void close(int handle);
void read_line(int handle, uint32_t* size, void** out);
void list(int handle);
uint32_t tellpos(int handle);
void seek(int handle, uint32_t offset, uint8_t where);
#endif
