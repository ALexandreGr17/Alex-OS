#include "mbr.h"
#include "disk.h"
#include "stdio.h"
#include <stdint.h>
#include "memory.h"

typedef struct {
    uint8_t attribute;
    uint8_t chs_addr_start[3];
    uint8_t partition_type;
    uint8_t chs_addr_end[3];
    uint32_t lba_partition_start;
    uint32_t size;
} __attribute__((packed)) MBR_entry_t;

void MBR_detect_partition(DISK* disk, void* partition, partition_t* partition_out){
    partition_out->disk = disk;
    if(disk->id < 0x80){
        partition_out->partition_offset = 0;
        partition_out->partition_size = (uint32_t)(disk->cylinders)
            *(uint32_t)(disk->heads)
            *(uint32_t)(disk->sectors);
        return;
    }

    MBR_entry_t* entry = (MBR_entry_t*)segoff_to_linear(partition);
    partition_out->partition_offset = entry->lba_partition_start;
    partition_out->partition_size = entry->size;
}

bool MBR_partition_read_sector(partition_t* disk, uint32_t lba, uint8_t sectors, void* data_out){
    return Disk_ReadSector(disk->disk, lba + disk->partition_offset, sectors, data_out);
}
