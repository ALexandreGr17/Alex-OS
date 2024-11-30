#include "elf.h"
#include "fat.h"
#include "mbr.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include <stdint.h>

const char ELF_MAGIC[] = { '\x7f', 'E', 'L', 'F' };

int ELF_open(partition_t *part, FAT_file *fd) {
    uint8_t *load_buffer = MEMORY_LOAD_KERNEL;
    uint32_t nb_read = FAT_Read(part, fd, sizeof(elf_hdr_t), load_buffer);
    if (nb_read != sizeof(elf_hdr_t)){
        printf("ELF load error read %x expected %x\n", nb_read, sizeof(elf_hdr_t));
        return 0;
    }
    elf_hdr_t *header = (elf_hdr_t*)load_buffer;
    if (memcmp(header->signature, ELF_MAGIC, 4)) {
        printf("ELF header is wrong\n");
        return 0;
    }
    if (header->bitness != ELF_BITNESS_32){
        printf("ELF bitness is wrong got: %x expected %x\n", header->bitness, ELF_BITNESS_32);
        return 0;
    }
    if (header->endianness != ELF_LITTLE_ENDIAN){
        printf("ELF endianness is wrong\n");
        return 0;
    }
    if (header->version != 1) {
        printf("wrong ELF version\n");
        return 0;
    }
    if (header->elf_version != 1) {
        printf("wrong ELF header version\n");
        return 0;
    }
    if (header->type != ELF_TYPE_EXECUTABLE) {
        printf("Not executable\n");
        return 0;
    }
    if (header->instruction_set != ELF_INSTRUCTION_SET_X86) {
        printf("wrong instruction_set\n");
        return 0;
    }

    uint32_t header_table_off = header->header_table;
    uint32_t nb_header_entry = header->header_entry_count;
    uint32_t to_read = header->header_entry_count * header->header_entry_size;
    if (header_table_off < nb_read) {
        nb_read += FAT_Read(part, fd, header_table_off - nb_read, load_buffer);
    }
    nb_read += FAT_Read(part, fd, to_read, load_buffer);
    for(int i = 0; i < nb_header_entry; i++){
        pgr_hdr_entry_t* pgr_hdr_entry = (pgr_hdr_entry_t*)load_buffer;
        if (!(pgr_hdr_entry->type & 1)){
            continue;
        }
        printf("type: 0x%lx\n", pgr_hdr_entry->type);
        printf("offset: 0x%lx\n", pgr_hdr_entry->p_offset);
        load_buffer += sizeof(pgr_hdr_entry_t);
    }
    FAT_Close(fd);
    return 1;
}
