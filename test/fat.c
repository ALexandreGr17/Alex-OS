#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

file_t root_dir;


void read_boot_sector(FILE* disk){
	fread(&boot_sector, sizeof(boot_sector_t), 1, disk);
}

void read_sector(FILE* disk, uint32_t lba, uint8_t nb_sector, uint8_t* out){
	fseek(disk, lba * boot_sector.bytes_per_sector, SEEK_SET);
	fread(out, boot_sector.bytes_per_sector, nb_sector, disk);
}

void write_sector(FILE* disk, uint32_t lba, uint8_t nb_sector, uint8_t* in){
	fseek(disk, lba * boot_sector.bytes_per_sector, SEEK_SET);
	fwrite(in, boot_sector.bytes_per_sector, nb_sector, disk);
}

uint32_t cluster_2_lba(uint32_t cluster){
	return ((cluster - 2) * boot_sector.sector_per_cluster) + first_data_sector;
}

void read_fat(FILE* disk, uint8_t offset){
	read_sector(disk, first_fat_sector + offset, 1, fat.table);
	fat.fat_offset = offset;
}

uint32_t next_cluster(FILE* disk, uint32_t current_cluster){
	uint32_t fat_offset = current_cluster + (current_cluster / 2);
	uint32_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

	if(ent_offset < fat.fat_offset * 512 || ent_offset >= fat.fat_offset * 512 + 512){
		read_fat(disk, ent_offset - (ent_offset % 512));
	}

	uint16_t val = *(uint16_t*)&fat.table[ent_offset];
	return current_cluster & 1 ? val >> 4 : val & 0x0FFF;
}

uint32_t find_free_cluster(FILE* disk){
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

#define MIN(x, y) x < y ? x : y

void read_file(FILE* disk, file_t* file, uint32_t count, uint8_t* out){
	count = MIN(count, file->size - file->position);
	uint32_t offset = file->position % 512;
	while(count > 0 && file->current_cluster < 0xFF8){
		uint32_t nb_read = MIN(count, 512);
		read_sector(disk, cluster_2_lba(file->current_cluster), 1, file->buffer);
		memcpy(out, file->buffer + offset, nb_read);
		out += nb_read - offset;
		file->position += nb_read - offset;
		count -= nb_read - offset;
		offset = 0;
		file->current_cluster = next_cluster(disk, file->current_cluster);
	}
}

void update_fat(FILE* disk, uint32_t current_cluster, uint32_t next_cluster){
	uint32_t fat_offset = current_cluster + (current_cluster / 2);
	uint16_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

	if(ent_offset < fat.fat_offset * 512 || ent_offset >= fat.fat_offset * 512 + 512){
		read_fat(disk, ent_offset - (ent_offset % 512));
	}
	uint16_t val = *(uint16_t*)&fat.table[ent_offset];
	uint16_t cluster =  next_cluster & 0x0FFF;
	if(current_cluster & 1){
		val = ((cluster << 4) & 0xFFF0) | (val & 0x000F);
	}
	else {
		val = cluster | (val & 0xF000);
	}
	*(uint16_t*)&fat.table[ent_offset] = val;
	write_sector(disk, first_fat_sector + fat.fat_offset, 1, fat.table);
}

void write_file(FILE* disk, file_t* file, uint32_t count, uint8_t* in){
	//count = MIN(count, 512);
	while(count > 0){
		uint16_t write_size = MIN(count, 512);
		memcpy(file->buffer, in, write_size);
		if(count != 512){
			memset(file->buffer + write_size, 0, 512 - write_size);
		}
		write_sector(disk, cluster_2_lba(file->current_cluster), 1, file->buffer);
		count -= write_size;
		in += write_size;
		file->position += write_size;
		uint32_t old_cluster = file->current_cluster;
		file->current_cluster = next_cluster(disk, file->current_cluster);
		if(file->current_cluster >= 0x0FF8){
			file->current_cluster = find_free_cluster(disk);
			update_fat(disk, old_cluster, file->current_cluster);
			update_fat(disk, file->current_cluster, 0x0FF8);
		}
	}
}

void seek_file(FILE* disk, file_t* file, uint32_t position){
	file->position = position;
	file->current_cluster = file->first_cluster;
	while(position > 512 && file->current_cluster < 0x0FF8){
		file->current_cluster = next_cluster(disk, file->current_cluster);
		position -= 512;
	}
}

void FAT_init(FILE* disk){
	read_boot_sector(disk);
	total_sectors = (boot_sector.total_sectors ? boot_sector.total_sectors : boot_sector.large_sector_coutn);
	fat_sectors = (boot_sector.sector_per_fat ? boot_sector.sector_per_fat : boot_sector.ebr_32.sector_per_fat);
	root_dir_sectors = ((boot_sector.root_dir_entry_count * sizeof(dir_entry_t)) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
	first_data_sector = boot_sector.reserved_sector_count + (boot_sector.nb_fat * fat_sectors) + root_dir_sectors;
	first_fat_sector = boot_sector.reserved_sector_count;
	data_sectors = total_sectors - (boot_sector.reserved_sector_count + (boot_sector.nb_fat * fat_sectors) + root_dir_sectors);
	total_clusters = data_sectors / boot_sector.sector_per_cluster;
	// FAT12
	first_root_dir_sector = first_data_sector - root_dir_sectors;


	root_dir.current_cluster = first_root_dir_sector;
	root_dir.first_cluster = first_root_dir_sector;
	memcpy(root_dir.filename, "ROOT", 4);
	root_dir.position = 0;
	root_dir.size = root_dir_sectors * boot_sector.bytes_per_sector;

	read_fat(disk, 0);
}

dir_entry_t entries[16];

void read_root_dir(FILE* disk){
	for(int i = 0; i < root_dir_sectors; i++){
		read_sector(disk, root_dir.current_cluster, 1, root_dir.buffer);
		for(int j = 0; j < 16; j++){
			memcpy(&entries[j], root_dir.buffer + (j * sizeof(dir_entry_t)), sizeof(dir_entry_t));
			if(entries[j].filename[0] == 0){
				return;
			}
			printf("filename: %s size: %d ", entries[j].filename, entries[j].size);
			if(entries[j].attributes & DIRECTORY){
				printf("d\n");
				continue;
			}
			if(entries[j].attributes & VOLUME_ID){
				printf("v\n");
				continue;
			}
			printf("\n");
		}
	}
}

void write_root_dir(FILE* disk, dir_entry_t* entry){
	if(root_dir.position >= root_dir.size - sizeof(dir_entry_t)){
		return;
	}
	for(int i = 0; i < root_dir_sectors; i++){
		read_sector(disk, root_dir.current_cluster, 1, root_dir.buffer);
		for(int j = 0; j < 16; j++){
			dir_entry_t entry_tmp;
			memcpy(&entry_tmp, root_dir.buffer + (j * sizeof(dir_entry_t)), sizeof(dir_entry_t));
			if(entry_tmp.filename[0] == 0){
				memcpy(root_dir.buffer + (j * sizeof(dir_entry_t)), entry, sizeof(dir_entry_t));
				printf("	0x%x\n", root_dir.current_cluster);
				write_sector(disk, root_dir.current_cluster, 1, root_dir.buffer);
				return;
			}
		}
	}

}

int main(int agrc, char** argv){
	if(agrc != 2){
		printf("usage: fat <path_to_img>\n");
		return 1;
	}

	FILE* disk = fopen(argv[1], "rb+");
	FAT_init(disk);
	read_root_dir(disk);

	file_t file = {
		.filename = "TEST    TXT",
		.first_cluster = entries[2].first_cluster_lo,
		.current_cluster = entries[2].first_cluster_lo,
		.position = 0,
		.size = entries[2].size,
	};

	char* buffer = calloc(1, file.size +1);
	//seek_file(disk, &file, 513);
	read_file(disk, &file, file.size, (uint8_t*)buffer);
	//printf("%s\n", buffer);

	uint32_t free_cluster = find_free_cluster(disk);
	//printf("0x%x\n", free_cluster);
	update_fat(disk, free_cluster, 0x0FF8);

	dir_entry_t new = {0};
	strcpy(new.filename, "TEST2   TXT");
	new.first_cluster_lo = free_cluster;

	write_root_dir(disk, &new);
	read_root_dir(disk);


	char* test = "test 1 2\n";
	file_t file2 = {
		.filename = "TEST2   TXT",
		.first_cluster = entries[3].first_cluster_lo,
		.current_cluster = entries[3].first_cluster_lo,
		.position = 0,
		.size = strlen(buffer),
	};

	size_t tmp = strlen(buffer);
	printf("file2 lba: 0x%x\n", cluster_2_lba(file2.current_cluster));
	write_file(disk, &file2, strlen(buffer), buffer);
	memset(buffer, 0, strlen(buffer));
	seek_file(disk, &file2, 0);
	read_file(disk, &file2, tmp, buffer);
	buffer[tmp] = 0;
	printf("%s\n", buffer);
	fclose(disk);
}
