#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <disk.h>
typedef struct {
	uint8_t	filename[11];
	uint8_t flags;
	uint8_t reserved;
	uint8_t creation_time_tenths;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t last_access_date;
	uint16_t first_cluster_high;
	uint16_t last_modification_time;
	uint16_t last_modification_date;
	uint16_t first_cluster_low;
	uint32_t file_size;
}FAT_dir_entry_t;

typedef struct {
	int handle;
	uint32_t size;
	uint32_t position;
	uint8_t is_dir;
} FAT_file_t;

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

void FAT_close(FAT_file_t* file);
uint8_t FAT_init(disk_t* disk);
FAT_file_t* FAT_open(disk_t* disk, const char* path);
uint32_t FAT_read(disk_t* disk, FAT_file_t* file, uint32_t byte_count, void* data_out);
uint8_t FAT_create(disk_t* disk, const char* filename);
void FAT_ls(disk_t* disk);

#endif
