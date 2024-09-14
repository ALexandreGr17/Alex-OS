#include "disk.h"
#include "memory/memory.h"
#include "string/string.h"
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <mem_management.h>
#include <filesystem/fat.h>

#define MAX_OPEN_FILE	50
#define ROOT_DIR_HANDLE -1
#define EOF				0xFF

typedef struct {
	uint8_t  drive_number;
	uint8_t  flags;
	uint8_t  signature;
	uint32_t volume_id;
	uint8_t  volume_label[11];
	uint8_t  system_id[8];
} __attribute__((packed)) ebr_t;

typedef struct {
	uint32_t sector_per_fat;
	uint16_t flags;
	uint16_t fat_version;
	uint32_t root_dir_cluster;
	uint16_t fs_info_sector;
	uint16_t backup_bs_sector;
	uint8_t  reserved[12];
	ebr_t    ebr;

} __attribute__((packed)) ebr_32_t;

typedef struct {
	uint8_t  jmp[3];
	uint8_t  OEM[8];
	uint16_t bytes_per_sector;
	uint8_t  sector_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t  nb_fat;
	uint16_t root_dir_entry_count;
	uint16_t total_sectors;
	uint8_t  media;
	uint16_t sector_per_fat;
	uint16_t sector_per_track;
	uint16_t nb_heads;
	uint32_t nb_hidden_sectors;
	uint32_t large_sector_coutn;
	union {
		ebr_32_t ebr_32;
		ebr_t    ebr;
	};

	uint8_t code[450];

} __attribute__((packed)) boot_sector_t;

typedef struct {
	uint8_t fat_offset;
	uint8_t table[512];
} fat_t;

typedef struct {
	uint8_t  filename[11];
	uint8_t  attributes;
	uint8_t  reserved;
	uint8_t  creation_time_tenths;
	uint16_t creation_time;
	uint16_t creation_date;
	uint16_t last_access_date;
	uint16_t first_cluster_hi;
	uint16_t last_modif_time;
	uint16_t last_modif_date;
	uint16_t first_cluster_lo;
	uint32_t size;
} __attribute__((packed)) dir_entry_t;

typedef enum {
	READ_ONLY = 0x01,
	HIDDEN    = 0x02,
	SYSTEM    = 0x04,
	VOLUME_ID = 0x08,
	DIRECTORY = 0x10,
	ARCHIVE   = 0x20,
	LFN       = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID
} dir_flag_t;

typedef struct {
	char filename[12];
	uint32_t position;
	uint32_t size;
	uint32_t first_cluster;
	uint32_t current_cluster;
	uint8_t  buffer[512];
	uint8_t  is_dir;
	uint8_t  open;
	int		 handle;
} file_t;

fat_t fat;
boot_sector_t boot_sector;
uint32_t total_sectors;
uint32_t fat_sectors;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t first_fat_sector;
uint32_t data_sectors;
uint32_t total_clusters;
uint32_t first_root_dir_sector;
uint8_t fat_type;

file_t root_dir;
file_t opened_files[MAX_OPEN_FILE];

uint8_t read_boot_sector(disk_t* disk){
	return disk->disk_read(disk->disk, 0, 1, (uint8_t*)&boot_sector);
}

uint8_t read_fat(disk_t* disk, uint8_t offset){
	fat.fat_offset = offset;
	return disk->disk_read(disk->disk, first_fat_sector + offset, 1, fat.table);
}

void FAT_detec(){
	if(boot_sector.sector_per_fat == 0){
		fat_type = 32;
	}
	else if(total_clusters < 4085){
		fat_type = 12;
	}
	else {
		fat_type = 16;
	}
}

uint32_t lba_2_cluster(uint32_t lba){
	return (lba - first_data_sector)/ boot_sector.sector_per_cluster + 2;
}

uint8_t FAT_init(disk_t* disk){
	if(read_boot_sector(disk)){
		printf("FAT init failed\n");
		errno = -1;
		return 0;
	}

	total_sectors = (boot_sector.total_sectors ? boot_sector.total_sectors : boot_sector.large_sector_coutn);
	fat_sectors = (boot_sector.sector_per_fat ? boot_sector.sector_per_fat : boot_sector.ebr_32.sector_per_fat);
	root_dir_sectors = ((boot_sector.root_dir_entry_count * sizeof(dir_entry_t)) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
	first_data_sector = boot_sector.reserved_sector_count + (boot_sector.nb_fat * fat_sectors) + root_dir_sectors;
	first_fat_sector = boot_sector.reserved_sector_count;
	data_sectors = total_sectors - (boot_sector.reserved_sector_count + (boot_sector.nb_fat * fat_sectors) + root_dir_sectors);
	total_clusters = data_sectors / boot_sector.sector_per_cluster;

	if(read_fat(disk, 0)){
		printf("FAT read fat failed\n");
		errno = -2;
		return 0;
	}
	
	FAT_detec();
	printf("FAT_type: %d\n", fat_type);

	// setting up root dir
	if(fat_type == 32){
		root_dir.first_cluster = boot_sector.ebr_32.root_dir_cluster;
	}
	else {
		root_dir.first_cluster = lba_2_cluster(first_data_sector - root_dir_sectors);
	}

	root_dir.current_cluster = root_dir.first_cluster;
	root_dir.filename[0] = '/';
	root_dir.position = 0;
	root_dir.size = root_dir_sectors * boot_sector.bytes_per_sector;
	root_dir.is_dir = 1;
	root_dir.open = 1;
	root_dir.handle = ROOT_DIR_HANDLE;

	errno = 0;
	return 1;
}

uint32_t next_cluster(disk_t* disk, uint32_t current_cluster){
	uint32_t fat_offset = current_cluster + (current_cluster / 2);

	switch (fat_type) {
		case 12:
			fat_offset = current_cluster + (current_cluster / 2);
			break;

		case 16:
			fat_offset = current_cluster * 2;
			break;

		case 32:
			fat_offset = current_cluster * 4;
			break;
		default:
			return 0;
	}

	uint32_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

	if(ent_offset < fat.fat_offset * 512 || ent_offset >= fat.fat_offset * 512 + 512){
		read_fat(disk, ent_offset - (ent_offset % 512));
	}

	uint32_t val;
	switch (fat_type){
		case 12:
			val = *(uint16_t*)&fat.table[ent_offset];
			if(current_cluster & 1){
				val = val >> 4;
			}
			else {
				val = val & 0x0FFF;
			}

			if(val >= 0x0FF8){
				val |= 0xFFFFF000;
			}
			return val;

		case 16:
			val = *(uint16_t*)&fat.table[ent_offset];
			if(val >= 0xFFF8){
				val |= 0xFFFF0000;
			}
			return val;

		case 32:
			val = *(uint32_t*)&fat.table[ent_offset];
			return val;

	}
	return 0;

}

#define MIN(x, y) x < y ? x : y

uint32_t cluster_2_lba(uint32_t cluster){
	return ((cluster - 2) * boot_sector.sector_per_cluster) + first_data_sector;
}

uint32_t find_free_cluster(disk_t* disk){
	if(fat.fat_offset != 0){
		read_fat(disk, 0);
	}
	for(int i = 2; i < total_clusters; i++){
		if(i > fat.fat_offset * 512 + 512){
			read_fat(disk, i - (i % 512));
		}
		uint32_t cluster = next_cluster(disk, i);
		if(cluster == 0){
			return i;
		}
	}
	return 0;
}

uint8_t update_fat(disk_t* disk, uint32_t current_cluster, uint32_t next_cluster){
	uint32_t fat_offset = current_cluster + (current_cluster / 2);

	switch (fat_type) {
		case 12:
			fat_offset = current_cluster + (current_cluster / 2);
			break;

		case 16:
			fat_offset = current_cluster * 2;
			break;

		case 32:
			fat_offset = current_cluster * 4;
			break;
		default:
			return 0;
	}

	uint32_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

	if(ent_offset < fat.fat_offset * 512 || ent_offset >= fat.fat_offset * 512 + 512){
		read_fat(disk, ent_offset - (ent_offset % 512));
	}

	uint32_t val;
	switch (fat_type){
		case 12:
			val = *(uint16_t*)&fat.table[ent_offset];
			next_cluster &= 0x0FFF;
			if(current_cluster & 1){
				val = ((next_cluster << 4) & 0xFFF0) | (val & 0x000F);
			}
			else {
				val = next_cluster | (val & 0xF000);
			}
			*(uint16_t*)&fat.table[ent_offset] = val;
			break;

		case 16:
			*(uint16_t*)&fat.table[ent_offset] = next_cluster & 0xFFFF;
			break;

		case 32:
			*(uint32_t*)&fat.table[ent_offset] = next_cluster;

	}
	disk->disk_write(disk->disk, first_fat_sector + fat_offset, 1, fat.table);
	return 1;
}

void debug_entry(dir_entry_t* entry){
	printf("######################################\n");
	printf("entry:\n");
	printf("	filename: %s\n", entry->filename);
	printf("	is_dir: %d\n", (entry->attributes & DIRECTORY) != 0);
	printf("	size: %i\n", entry->size);
	printf("	first_cluster: 0x%x\n", (entry->first_cluster_hi << 16) | entry->first_cluster_lo);
	
	printf("\n");
}

void debug_file(file_t* file){
	printf("######################################\n");
	printf("file:\n");
	printf("	filename: %s\n", file->filename);
	printf("	is_dir: %d\n", file->is_dir);
	printf("	size: %i\n", file->size);
	printf("	handle: %i\n", file->handle);
	printf("	first_cluster: 0x%x\n", file->first_cluster);
	printf("	current_cluster: 0x%x\n", file->current_cluster);
	printf("	position: %i\n", file->position);
	printf("	lba_start: 0x%x\n", cluster_2_lba(file->first_cluster) * 512);
	
	printf("\n");

};

uint32_t FAT_read(disk_t* disk, int handle, uint32_t count, uint8_t* out){
	file_t* file = handle == ROOT_DIR_HANDLE ? &root_dir : &opened_files[handle - 3];
	if(!file->is_dir){
		count = MIN(count, file->size - file->position);
	}
	uint32_t nb_read = 0;
	uint32_t offset = file->position % 512;
	while (count > 0 && file->current_cluster < 0xFFFFFFF8) {
		uint16_t nb_to_read = MIN(count + offset, 512);
		disk->disk_read(disk->disk, cluster_2_lba(file->current_cluster), 1, file->buffer);
		memcpy(out, file->buffer + offset, nb_to_read);
		out += nb_to_read - offset;
		nb_read = nb_to_read - offset;
		file->position += nb_to_read - offset;
		count -= nb_to_read - offset;
		offset = 0;
		if(nb_to_read >= 512){
			file->current_cluster = next_cluster(disk, file->current_cluster);
		}
	}
	return nb_read;
}

uint32_t FAT_write(disk_t* disk, int handle, uint32_t count, uint8_t* in){
	file_t* file = handle == ROOT_DIR_HANDLE ? &root_dir : &opened_files[handle - 3];
	uint32_t writed_size = 0;
	while (count > 0) {
		uint16_t write_size = MIN(512, count);
		memcpy(file->buffer, in, write_size);
		if(count < 512){
			memset(file->buffer + write_size, 0, 512 - write_size);
		}

		if(disk->disk_write(disk->disk, cluster_2_lba(file->current_cluster), 1, file->buffer)){
			return writed_size;
		}
		count -= write_size;
		file->position += write_size;
		file->size += write_size;
		in += write_size;
		writed_size += write_size;

		uint32_t old_cluster = file->current_cluster;
		file->current_cluster = next_cluster(disk, file->current_cluster);
		if(file->current_cluster >= 0xFFFFFFF8){
			file->current_cluster = find_free_cluster(disk);
			update_fat(disk, old_cluster, file->current_cluster);
			update_fat(disk, file->current_cluster, 0xFFFFFFF8);
		}
	}
	return writed_size;
}

void FAT_seek(disk_t* disk, int handle, uint32_t offset, uint8_t where){
	file_t* file = handle == ROOT_DIR_HANDLE ? &root_dir : &opened_files[handle - 3];
	if(where == SEEK_END){
		file->current_cluster = file->first_cluster;
		while(file->current_cluster < 0xFFFFFFF8){
			disk->disk_read(disk->disk, cluster_2_lba(file->current_cluster), 1, file->buffer);
			int i = 0;
			while(i < 512){
				if(file->is_dir){
					if(file->buffer[i] == 0){
						file->size = file->position;
						return;
					}
					i += sizeof(dir_entry_t);
					file->position += sizeof(dir_entry_t);
				}
				else {
					if(file->buffer[i] == EOF){
						file->size = file->position;
						return;
					}
					i++;
					file->position++;
				}
			}
		}
	}

	if(where == SEEK_CUR){
		file->position += offset;
		offset += file->position;
	}
	else {
		file->position = offset;
	}
	file->current_cluster = file->first_cluster;

	while (offset > 512 && file->current_cluster < 0xFFFFFFF8) {
		file->current_cluster = next_cluster(disk, file->current_cluster);
		offset -= MIN(offset, 512);
	}
}

uint32_t FAT_append(disk_t* disk, int handle, uint32_t count, uint8_t* in){
	file_t* file = handle == ROOT_DIR_HANDLE ? &root_dir : &opened_files[handle - 3];
	uint32_t writed_size = 0;
	FAT_seek(disk, handle, 0, SEEK_END);
	uint16_t offset = file->position % 512;
	while(count > 0){
		uint16_t to_write = MIN(count, 512 - offset);
		disk->disk_read(disk->disk, cluster_2_lba(file->current_cluster), 1, file->buffer);
		memcpy(file->buffer + offset, in, to_write);
		disk->disk_write(disk->disk, cluster_2_lba(file->current_cluster), 1, file->buffer);

		offset = 0;
		count -= to_write;
		in += to_write;
		file->position += to_write;
		file->size += to_write;
		writed_size += to_write;
		if(to_write >= 512){
			uint32_t old_cluster = file->current_cluster;
			file->current_cluster = next_cluster(disk, file->current_cluster);
			if(file->current_cluster > 0xFFFFFFF8){
				file->current_cluster = find_free_cluster(disk);
				update_fat(disk, file->current_cluster, 0xFFFFFFF8);
				update_fat(disk, old_cluster, file->current_cluster);
			}
		}
	}
	return writed_size;
}

uint8_t read_next_entry(disk_t* disk, file_t* file, dir_entry_t* entry_out){
	return FAT_read(disk, file->handle, sizeof(dir_entry_t), (uint8_t*)entry_out) == sizeof(dir_entry_t);
}

dir_entry_t* find_file(disk_t* disk, file_t* dir, char* filename){
	if(strlen(filename) > 11){
		errno = -1;
		return NULL;
	}

	char FAT_name[11];
	memset(FAT_name, ' ', 11);
	char* ext = strchr(filename, '.');


	if(ext != NULL){
		for(int i = 0; i < 11 && filename + i < ext; i++){
			FAT_name[i] = toupper(filename[i]);
		}
		int ext_len = strlen(ext);
		for(int i = 1; i < ext_len; i++){
			FAT_name[i + 11 - ext_len] = toupper(ext[i]);
		}
	}
	else {
		for(int i = 0; filename[i] && i < 11; i++){
			FAT_name[i] = toupper(filename[i]);
		}
	}
	
	dir_entry_t* entry = malloc(sizeof(dir_entry_t));
	do {
		if(!read_next_entry(disk, dir, entry)){
			errno = -2;
			return NULL;
		}
		if(memcmp(entry->filename, FAT_name, 11)){
			return entry;
		}
	}while(entry->filename[0] != 0);

	free(entry);
	errno = 0;
	return NULL;
}

int find_free_handle(){
	for(int i = 0; i < MAX_OPEN_FILE; i++){
		if(!opened_files[i].open){
			return i + 3;
		}
	}
	return 0;
}

int FAT_open(disk_t* disk, char* path){
	if(path == NULL){
		errno = -1;
		return 0;
	}


	file_t* dir = &root_dir;
	dir->current_cluster = dir->first_cluster;
	dir->position = 0;
	if(*path == '/'){
		path++;
	}

	int handle = find_free_handle();
	if(handle == 0){
		errno = -2;
		return 0;
	}

	file_t* file = &opened_files[handle - 3];
	file->handle = handle;

	char* next_dir = strchr(path, '/');
	dir_entry_t* entry;
	do {
		if(next_dir != NULL){
			uint16_t size_name = next_dir - path;
			path[size_name] = 0;
		}

		if(!dir->is_dir){
			errno = -1;
			return -1;
		}

		entry = find_file(disk, dir, path);
		if(!entry){
			errno = -1;
			return -1;
		}

		
		file->first_cluster = entry->first_cluster_hi << 16 | entry->first_cluster_lo;
		file->current_cluster = file->first_cluster;
		file->is_dir = (entry->attributes & DIRECTORY) != 0;
		file->size = entry->size;
		file->position = 0;
		file->handle = handle;
		memcpy(file->filename, entry->filename, 11);
		free(entry);
		path = next_dir;
		if(path != NULL){
			path++;
		}
		next_dir = strchr(path, '/');
		dir = file;
		dir->handle = file->handle;
	}while(path != NULL);

	file->open = 1;
	return handle;
}

char* parse_filename_create_file(char* filename){
	int last_pos = 0;
	for(int i = 0; filename[i]; i++){
		if(filename[i] == '/'){
			last_pos = i;
		}
	}
	filename[last_pos] = 0;
	return filename + last_pos + 1;
}

void normal_to_fat_name(char* in, char* out){
	memset(out, ' ', 11);
	char* ext = strchr(in, '.');
	if(ext != NULL){
		for(int i = 0; i < 11 && in + i < ext; i++){
			out[i] = toupper(in[i]);
		}
		int ext_len = strlen(ext);
		for(int i = 1; i < ext_len; i++){
			out[i + 11 - ext_len] = toupper(ext[i]);
		}
	}
	else {
		for(int i = 0; in[i] && i < 11; i++){
			out[i] = toupper(in[i]);
		}
	}
	

}

uint8_t FAT_create_file(disk_t* disk, char* path){
	errno = 0;
	char* file_name = parse_filename_create_file(path);
	int d_hd = FAT_open(disk, path);
	FAT_seek(disk, d_hd, 0, SEEK_SET);


	if(d_hd < 0){
		errno = -1;
		return 0;
	}
	
	uint32_t new_cluster = find_free_cluster(disk);
	update_fat(disk, new_cluster, 0xFFFFFFF8);
	dir_entry_t entry = {0};
	normal_to_fat_name(file_name, entry.filename);
	entry.attributes = 0;
	entry.size = 0;
	entry.first_cluster_hi = (new_cluster >> 16) & 0xFFFF;
	entry.first_cluster_lo = new_cluster & 0xFFFF;

	FAT_append(disk, d_hd, sizeof(dir_entry_t), &entry);
	
	return 1;
}
