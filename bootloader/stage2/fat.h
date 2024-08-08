#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include "disk.h"

typedef struct {
	uint8_t		file_name[11];
	uint8_t		file_attribute;
	uint8_t		_Reserved;
	uint8_t		create_time_tenths;
	uint16_t	create_time;
	uint16_t	create_date;
	uint16_t	last_access_date;
	uint16_t	first_cluster_high;	// not used in fat12/16
	uint16_t	last_modified_time;
	uint16_t	last_modified_date;
	uint16_t	first_cluster_low;
	uint32_t	file_size;
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct {
	int Handle;
	bool isDirectory;
	uint32_t Position;
	uint32_t Size;
} FAT_file;

enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool FAT_Initialize(DISK* disk);
FAT_file* FAT_Open(DISK* disk, const char* path);
uint32_t FAT_Read(DISK* disk, FAT_file* file, uint32_t byteCount, void* output_buffer);
bool FAT_ReadEntry(DISK* disk, FAT_file* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_file* file);
#endif
