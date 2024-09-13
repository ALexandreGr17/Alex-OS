#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <disk.h>

uint8_t FAT_init(disk_t* disk);

int FAT_open(disk_t* disk, char* path);
uint32_t FAT_read(disk_t* disk, int handle, uint32_t count, uint8_t* out);
uint32_t FAT_write(disk_t* disk, int handle, uint32_t count, uint8_t* in);
#endif
