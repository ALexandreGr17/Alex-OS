#include "fat.h"
#include <disk.h>
#include <stdint.h>
#include <stdio.h>
#include <string/string.h>
#include <mem_management.h>
#include <memory/memory.h>
#include <stddef.h>

#define MAX_PATH_SIZE 256
#define SECTOR_SIZE		512
#define FAT_CACHE_SIZE	5
#define MAX_FILE_HANDLES 10
#define ROOT_DIR_HANDLE -1

#define MIN(x, y) x < y ? x : y




typedef struct {
	uint8_t		drive_number;
	uint8_t		reserved;
	uint8_t		signature;
	uint32_t	volume_id;
	uint8_t		volume_label[11];
	uint8_t     fat_label[8];
} __attribute__((packed)) FAT_extBS_12_16_t;

typedef struct {
	uint32_t	sector_per_fat;
	uint16_t	flags;
	uint16_t	fat_version;
	uint32_t	root_cluster;
	uint16_t	fat_info_cluster;
	uint16_t	backup_BS_sector;
	uint8_t		reserved[12];
	FAT_extBS_12_16_t extBS;
} __attribute__((packed)) FAT_extBS_32_t;

typedef struct {
	uint8_t		boot_jump[3];
	uint8_t		oem_name[8];
	uint16_t	bytes_per_sector;
	uint8_t		sector_per_cluster;
	uint16_t	reserved_sector_count;
	uint8_t		fat_count;
	uint16_t	root_entry_count;
	uint16_t	total_sectors_16;
	uint8_t		media_type;
	uint16_t	sector_per_fat;
	uint16_t	sector_per_track;
	uint16_t	head_count;
	uint32_t	hidden_sector_count;
	uint32_t	total_sectors_32;

	union {
		FAT_extBS_12_16_t extBS_12_16;
		FAT_extBS_32_t extBS_32;
	};
} __attribute__((packed)) FAT_boot_sector_t;

typedef struct {
	uint8_t buffer[SECTOR_SIZE];
	FAT_file_t file;
	uint8_t open;
	uint32_t first_cluster;
	uint32_t current_cluster;
	uint32_t current_sector;
} FAT_file_data_t;

typedef struct {
	union {
		FAT_boot_sector_t boot_sector;
		uint8_t boot_sector_bytest[512];
	};

	uint8_t FAT_cache[FAT_CACHE_SIZE * SECTOR_SIZE];
	uint32_t FAT_cache_index;

	FAT_file_data_t root_dir;

	FAT_file_data_t files[MAX_FILE_HANDLES];

} FAT_data_t;

FAT_data_t* FAT_data;
uint32_t total_sectors;
uint32_t sector_per_fat;
uint32_t data_section_lba;
uint8_t fat_type;

uint8_t FAT_read_boot_sector(disk_t* disk){
    //printf("0x%x\n", sizeof(FAT_data->boot_sector_bytest));
	return !disk->disk_read(disk->disk, 0, 1, FAT_data->boot_sector_bytest);
}

uint32_t FAT_cluster_2_lba(uint32_t cluster){
	return data_section_lba + (cluster - 2) * FAT_data->boot_sector.sector_per_cluster;
}

void FAT_detect(){
	uint32_t data_cluster = (total_sectors - data_section_lba) / FAT_data->boot_sector.sector_per_cluster;
	if(data_cluster < 0xFF5 ){
		fat_type = 12;
	}
	else {
		if(FAT_data->boot_sector.sector_per_fat){
			fat_type = 16;
		}
		else {
			fat_type = 32;
		}
	}
}

uint8_t FAT_init(disk_t* disk){
	FAT_data = malloc(sizeof(FAT_data_t));
	//printf("0x%x\n", sizeof(FAT_data_t));

	if(!FAT_read_boot_sector(disk)){
		printf("FAT: read boot sector failed\n");
		return -1;
	}
	
	FAT_data->FAT_cache_index = 0xFFFFFFFF;
	total_sectors = FAT_data->boot_sector.total_sectors_16;
	if(total_sectors == 0){
		total_sectors = FAT_data->boot_sector.total_sectors_32;
	}

	sector_per_fat = FAT_data->boot_sector.sector_per_fat;
	if(sector_per_fat == 0){
		sector_per_fat = FAT_data->boot_sector.extBS_32.sector_per_fat;
	}

	uint32_t root_dir_size;
	uint32_t root_dir_lba;

	if(FAT_data->boot_sector.sector_per_fat){
		// FAT 12 16
		root_dir_lba = FAT_data->boot_sector.reserved_sector_count + FAT_data->boot_sector.sector_per_fat * FAT_data->boot_sector.fat_count;
		root_dir_size = sizeof(FAT_dir_entry_t) * FAT_data->boot_sector.root_entry_count;
		uint32_t root_dir_sec = (root_dir_size + FAT_data->boot_sector.bytes_per_sector - 1) / FAT_data->boot_sector.bytes_per_sector;
		data_section_lba = root_dir_lba + root_dir_sec;
	}
	else {
		// FAT 32
		root_dir_lba = FAT_cluster_2_lba(FAT_data->boot_sector.extBS_32.root_cluster);
		root_dir_size = 0;
		data_section_lba = FAT_data->boot_sector.reserved_sector_count + FAT_data->boot_sector.fat_count * FAT_data->boot_sector.sector_per_fat;
	}

	FAT_data->root_dir.file.handle = ROOT_DIR_HANDLE;
	FAT_data->root_dir.open = 1;
	FAT_data->root_dir.file.is_dir = 1;
	FAT_data->root_dir.file.size = root_dir_size;
	FAT_data->root_dir.first_cluster = root_dir_lba;
	FAT_data->root_dir.current_cluster = root_dir_lba;
	FAT_data->root_dir.current_sector = 0;

	if(disk->disk_read(disk->disk, root_dir_lba, 1 ,FAT_data->root_dir.buffer)){
		printf("FAT: can't read Root dir\n");
		return -2;
	}

	FAT_detect();

	for(int i = 0; i < MAX_FILE_HANDLES; i++){
		FAT_data->files[i].open = 0;
	}

	printf("root_dir_lba: 0x%x\n", root_dir_lba);

	return 0;
}


FAT_file_t* FAT_open_entry(disk_t* disk, FAT_dir_entry_t* entry){
	int handle = -1;
	for(int i = 0; i < MAX_FILE_HANDLES; i++){
		if(!FAT_data->files[i].open){
			handle = i;
		}
	}

	if(handle < 0){
		printf("FAT: no more handle\n");
		return NULL;
	}

	FAT_file_data_t* file_data = &FAT_data->files[handle];
	file_data->file.handle = handle;
	file_data->file.is_dir = (entry->flags & FAT_ATTRIBUTE_DIRECTORY) != 0;
	file_data->file.size = entry->file_size;
	file_data->first_cluster = (entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16));
	file_data->current_cluster = file_data->first_cluster;
	file_data->current_sector = 0;

	if(disk->disk_read(disk->disk, FAT_cluster_2_lba(file_data->current_cluster), 1, file_data->buffer) < 0){
		printf("FAT: open entry failed - read error cluster=%u lba=%u\n", 
				file_data->current_cluster, FAT_cluster_2_lba(file_data->current_cluster));
		return NULL;
	}
	file_data->open = 1;
	return &file_data->file;
}

uint8_t FAT_read_fat(disk_t* disk, uint32_t index_lba){
	return disk->disk_read(disk->disk, FAT_data->boot_sector.reserved_sector_count + index_lba, FAT_CACHE_SIZE, FAT_data->FAT_cache);
}

uint32_t FAT_next_cluster(disk_t* disk, uint32_t current_cluster){
	uint32_t fat_index = 0;

	if(fat_type == 12){
		fat_index = current_cluster * 3 / 2;
	}
	else if (fat_index == 16){
		fat_index = current_cluster * 2;
	}

	else {
		fat_index = current_cluster * 4;
	}

	uint32_t fat_index_sector = fat_index / SECTOR_SIZE;
	
	if(fat_index_sector < FAT_data->FAT_cache_index 
			|| fat_index_sector >= FAT_data->FAT_cache_index + FAT_CACHE_SIZE) {
		FAT_read_fat(disk, fat_index_sector);
		FAT_data->FAT_cache_index = fat_index_sector;
	}

	fat_index -= (FAT_data->FAT_cache_index * SECTOR_SIZE);
	uint32_t next_cluster;
	if(fat_type == 12){
		if(current_cluster % 2 == 0){
			next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index)) & 0x0FFF;
		}
		else {
			next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index)) >> 4;	
		}

		if(next_cluster >= 0x0FF8){
			next_cluster |= 0xFFFFF000;	
		}
	}

	else if (fat_type == 16){
		next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index));
		if(next_cluster >= 0xFFF8){
			next_cluster |= 0xFFFF0000;
		}
	}

	else {
		next_cluster = (*(uint32_t*)(FAT_data->FAT_cache + fat_index));
	}
	return next_cluster;
}


uint32_t FAT_read(disk_t* disk, FAT_file_t* file, uint32_t byte_count, void* data_out){
	FAT_file_data_t* file_data = (file->handle == ROOT_DIR_HANDLE) ? &FAT_data->root_dir : &FAT_data->files[file->handle];
	
	if(!file_data->file.is_dir || (file_data->file.is_dir && file_data->file.size != 0)){
		byte_count = MIN(byte_count, file_data->file.size - file_data->file.position);
	}

	uint8_t* buffer = data_out;

	while(byte_count > 0){
		uint32_t left_in_buffer = SECTOR_SIZE - (file_data->file.position % SECTOR_SIZE);
		uint32_t take = MIN(byte_count, left_in_buffer);
		memcpy(buffer, file_data->buffer + file_data->file.position % SECTOR_SIZE, take);	
		buffer += take;
		file_data->file.position += take;
		byte_count -= take;

		if(left_in_buffer == take){
			if(file_data->file.handle == ROOT_DIR_HANDLE){
				file_data->current_cluster++;	

				if(disk->disk_read(disk->disk, file_data->current_cluster, 1, file_data->buffer) < 0){
					printf("FAT: read error 1\n");
					break;
				}
			}

			else {
				file_data->current_sector++;
				if(file_data->current_cluster >= FAT_data->boot_sector.sector_per_cluster){
					file_data->current_sector = 0;	
					file_data->current_cluster = FAT_next_cluster(disk, file_data->current_cluster);
				}

				if(file_data->current_cluster >= 0x0FF8){
					file_data->file.size = file_data->file.position;
					break;
				}

				if(disk->disk_read(disk->disk, FAT_cluster_2_lba(file_data->current_cluster) + file_data->current_sector, 1, file_data->buffer) < 0){
					printf("FAT: read error 2\n");
					break;
				}
			}

		}
	}

	return buffer - (uint8_t*)data_out;
}

uint8_t FAT_read_entry(disk_t* disk, FAT_file_t* file, FAT_dir_entry_t* entry){
	return FAT_read(disk, file, sizeof(FAT_dir_entry_t), entry) == sizeof(FAT_dir_entry_t);
}


void FAT_close(FAT_file_t* file){
	if(file->handle == ROOT_DIR_HANDLE){
		file->position = 0;
		FAT_data->root_dir.current_cluster = FAT_data->root_dir.first_cluster;
	}
	FAT_data->files[file->handle].open = 0;
}

uint8_t FAT_find_file(disk_t* disk, FAT_file_t* file, const char* name, FAT_dir_entry_t* entry_out){
	char FAT_name[12];
	FAT_dir_entry_t entry;
	memset(FAT_name, ' ', 12);
	FAT_name[11] = 0;
	const char* ext = strchr(name, '.');
	if(!ext){
		ext = name + 11;
	}

	for(int i = 0; i < 8 && name[i] && name + i < ext; i++){
		FAT_name[i] = toupper(name[i]);
	}

	if(ext != name + 11){
		for(int i = 0; i < 3 && ext[i]; i++){
			FAT_name[i + 8] = toupper(ext[i+1]);
		}
	}

	while(FAT_read_entry(disk, file, &entry)){
		if(memcmp(FAT_name, entry.filename, 11)){
			*entry_out = entry;
			//printf("%s %s\n\n", entry.filename, FAT_name);
			return 1;
		}
	}
	return 0;
}


FAT_file_t* FAT_open(disk_t* disk, const char* path){
	char name[MAX_PATH_SIZE];
	if(path[0] == '/'){
		path++;
	}

	FAT_file_t* current = &FAT_data->root_dir.file;
	while(*path){
		uint8_t is_last = 0;	
		const char* delim = strchr(path, '/');
		if(delim != NULL){
			memcpy(name, path, delim - path);
			name[delim - path] = 0;
			path = delim + 1;
		}
		else {
			unsigned len = strlen(path);
			memcpy(name, path, len);
			name[len] = 0;
			path += len;
			is_last = 1;
		}
		
		FAT_dir_entry_t entry;
		if(FAT_find_file(disk, current, name, &entry)){
			FAT_close(current);	
			if(!is_last && (entry.flags | FAT_ATTRIBUTE_DIRECTORY) == 0){
				printf("FAT: file %s is a directory\n", name);
				return NULL;
			}
			current = FAT_open_entry(disk, &entry);
		}
		else {
			FAT_close(current);
			printf("FAT: file %s not found\n", name);
			return NULL;
		}
		
	}
	return current;
}

uint8_t FAT_is_cluster_empty(disk_t* disk, uint32_t current_cluster){
	uint32_t fat_index = 0;

	if(fat_type == 12){
		fat_index = current_cluster * 3 / 2;
	}
	else if (fat_index == 16){
		fat_index = current_cluster * 2;
	}

	else {
		fat_index = current_cluster * 4;
	}

	uint32_t fat_index_sector = fat_index / SECTOR_SIZE;
	
	if(fat_index_sector < FAT_data->FAT_cache_index 
			|| fat_index_sector >= FAT_data->FAT_cache_index + FAT_CACHE_SIZE) {
		FAT_read_fat(disk, fat_index_sector);
		FAT_data->FAT_cache_index = fat_index_sector;
	}

	fat_index -= (FAT_data->FAT_cache_index * SECTOR_SIZE);
	uint32_t next_cluster;
	if(fat_type == 12){
		if(current_cluster % 2 == 0){
			next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index)) & 0x0FFF;
		}
		else {
			next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index)) >> 4;	
		}
		
	}

	else if (fat_type == 16){
		next_cluster = (*(uint16_t*)(FAT_data->FAT_cache + fat_index));
	}

	else {
		next_cluster = (*(uint32_t*)(FAT_data->FAT_cache + fat_index));
	}
	return next_cluster == 0;
}

uint32_t FAT_find_empty_cluster(disk_t* disk){
	uint32_t current_cluster = 2;
	uint32_t nb_cluster = (sector_per_fat  * SECTOR_SIZE) / fat_type ;
	
	while(current_cluster < nb_cluster){
		printf("current_cluster: %d\n", current_cluster);
		if(FAT_is_cluster_empty(disk, current_cluster)){
			return current_cluster;
		}
		current_cluster++;
	}
	return 0;
}

uint8_t FAT_test_file(disk_t* disk, const char* filename){
	if(strlen(filename) >= 11){
		return -1;
	}

	char bad_chars[] = "!@%^*~|";
	for(int i = 0; filename[i]; i++){
		for(int j = 0; j < sizeof(bad_chars); j++){
			if(filename[i] == bad_chars[j]){
				return -2;
			}
		}
	}
	return 0;
}
/*
uint8_t FAT_is_filename_valide(disk_t* disk, const char* filename){
	char FAT_file[12] = {0};
	if(*filename == '/'){
		filename++;
	}

	do {
		char* filename_new = strchr(filename, '/');
		memcpy(FAT_file, filename_new, filename_new - filename);

	}
}
*/

void FAT_ls(disk_t* disk){
	FAT_dir_entry_t entry;
	FAT_data->root_dir.current_sector = 0;
	FAT_data->root_dir.current_cluster = FAT_data->root_dir.first_cluster;
	FAT_data->root_dir.file.position = 0;
	while(FAT_read_entry(disk, &FAT_data->root_dir.file, &entry)){
		char file[12] = {0};
		memcpy(file, entry.filename, 11);
		if(file[0] == 0 || entry.file_size == -1){
			continue;
		}
		if(entry.flags & FAT_ATTRIBUTE_DIRECTORY){
			printf("d  %s, %d\n", file, entry.file_size);
		}
		else {
			printf("   %s, %d\n", file, entry.file_size);
		}
	};
}

uint8_t FAT_write(disk_t* disk, FAT_file_t* file, uint32_t size, void* data_in){
	FAT_file_data_t* fd = file->handle == ROOT_DIR_HANDLE ? &FAT_data->root_dir : &FAT_data->files[file->handle];
	
	uint8_t* buffer = data_in;

	int nb_sector = size / SECTOR_SIZE + (size % SECTOR_SIZE != 0 ? 1 : 0);
	if(nb_sector < FAT_data->boot_sector.sector_per_cluster){
		// un seul cluster a ecrire
		printf("nb_sector %i\n", nb_sector);
		uint8_t* buffer_write = malloc(nb_sector * FAT_data->boot_sector.bytes_per_sector);
		memset(buffer_write, 0, nb_sector * FAT_data->boot_sector.bytes_per_sector);
		memcpy(buffer_write, buffer, size);
		printf("lba: 0x%x\n", FAT_cluster_2_lba(fd->current_cluster));
		if(disk->disk_write(disk->disk, fd->current_cluster, nb_sector, buffer_write) < 0){
			printf("FAT: write failed cluster: %u, sector: %u\n", fd->current_cluster, FAT_cluster_2_lba(fd->current_cluster));
			return -1;
		}
	}
	return 0;
}

uint8_t FAT_write_entry(disk_t* disk, FAT_file_t* file, FAT_dir_entry_t* entry){
	return FAT_write(disk, file, sizeof(FAT_dir_entry_t), entry);
}

uint8_t FAT_create(disk_t* disk, const char* filename){
	
	char FAT_name[12] = {0};
	memset(FAT_name, ' ', 11);
	const char* ext = strchr(filename, '.');
	if(!ext){
		ext = filename + 11;
	}

	for(int i = 0; i < 8 && filename[i] && filename + i < ext; i++){
		FAT_name[i] = toupper(filename[i]);
	}

	if(ext != filename + 11){
		for(int i = 0; i < 3 && ext[i]; i++){
			FAT_name[i + 8] = toupper(ext[i+1]);
		}
	}

	printf("%s\n", FAT_name);

	FAT_file_t* root_dir = &FAT_data->root_dir.file;
	FAT_dir_entry_t entry;
	do {
		FAT_read(disk, root_dir, sizeof(FAT_dir_entry_t), &entry);
		char FAT_name[12] = {0};
		memcpy(FAT_name, entry.filename, 11);
		printf("%s\n", FAT_name);
	}while(entry.filename[0] != 0);
	FAT_dir_entry_t new_entry = {0};
	memcpy(new_entry.filename, FAT_name, 11);
	uint32_t cluster = FAT_find_empty_cluster(disk);
	printf("cluster: %u\n", cluster);
	new_entry.first_cluster_low = (uint16_t)(cluster & 0xFFFF);
	new_entry.first_cluster_high = (uint16_t)((cluster >> 16) & 0xFFFF);
	//return FAT_write_entry(disk, root_dir, &new_entry);
	return 0;
}
