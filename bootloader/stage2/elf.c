#include "elf.h"
#include "fat.h"
#include "mbr.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include <stdint.h>

const char ELF_MAGIC[] = { '\x7f', 'E', 'L', 'F' };

int elf_validate_header(elf_hdr_t* header) {
    if (memcmp(header->signature, ELF_MAGIC, 4)) {
        printf("ELF header is wrong\n");
        for(int i = 0; i < 4; i++) {
            printf("0x%x, %c\n", header->signature[i], header->signature[i]);
        }
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
    return 1;
}

#define NULL 0
#define MIN(x, y) x > y ? y : x

void debug_elf_hdr(elf_hdr_t* hdr) {
    printf("##### ELF HEADER #####\n");
    printf("\tmagic: 0x%x %c %c %c\n", hdr->signature[0], hdr->signature[1], hdr->signature[2], hdr->signature[3]);
    printf("\tbitness: 0x%x\n", hdr->bitness);
    printf("\tendianess: 0x%x\n", hdr->endianness);
    printf("\tversion: 0x%x\n", hdr->version);
    printf("\tabi: 0x%x\n", hdr->abi);
    printf("\ttype: 0x%x\n", hdr->type);
    printf("\tinstruction set: 0x%x\n", hdr->instruction_set);
    printf("\telf_version: 0x%x\n", hdr->elf_version);
    printf("\tentry_point: 0x%x\n", hdr->program_entry);
}

int ELF_read(partition_t* part, char* filename, void** entry_point) {
    uint8_t* load_buf = MEMORY_LOAD_KERNEL;
    uint8_t* header_buf = MEMORY_ELF_ADDR;
    uint32_t file_pos = 0;
    uint32_t read;
    FAT_file* fd = FAT_Open(part, filename);
    if(fd == NULL) {
        return 0;
    }
    // read header
    if((read = FAT_Read(part, fd, sizeof(elf_hdr_t), header_buf)) != sizeof(elf_hdr_t)) {
        printf("ELF load error\n");
        FAT_Close(fd);
        return 0;
    }
    file_pos += read;
    elf_hdr_t* header = (elf_hdr_t*)header_buf;
    //debug_elf_hdr(header);
    if (!elf_validate_header(header)) {
        FAT_Close(fd);
        return 0;
    }
    *entry_point = (void*)header->program_entry;

    // load program header
    uint32_t prg_hdr_offset = header->header_table;
    uint32_t prg_hdr_size = header->prg_header_table_entry_size * header->prg_header_table_entry_count;
    uint32_t prg_hdr_table_entry_size = header->prg_header_table_entry_size;
    uint32_t prg_hdr_table_entry_count = header->prg_header_table_entry_count;

    file_pos += FAT_Read(part, fd, prg_hdr_offset - file_pos, header_buf);
    if((read = FAT_Read(part, fd, prg_hdr_size, header_buf)) != prg_hdr_size) {
        printf("ELF load program header Failed\n");
        FAT_Close(fd);
        return 0;
    }
    file_pos += read;

    FAT_Close(fd);  // closing beacaus cant do a seek

    for(uint32_t i = 0; i < prg_hdr_table_entry_count; i++) {
        elf_pgr_hdr_entry_t* prg_header = (elf_pgr_hdr_entry_t*)(header_buf + i * prg_hdr_table_entry_size);
        if (prg_header->type == ELF_PROGRAM_TYPE_LOAD) {
            uint8_t* virt_addr = (uint8_t*)prg_header->p_vaddr;
            memset(virt_addr, 0, prg_header->p_memsz);

            fd = FAT_Open(part, filename);  // seek to pos 0
            while(prg_header->p_offset > 0) {
                uint32_t should_read = MIN(prg_header->p_offset, MEMORY_LOAD_SIZE);
                read = FAT_Read(part, fd, should_read, load_buf);
                if (read != should_read) {
                    printf("ELF seek program Error : read %x should read %x\n", read, should_read);
                    FAT_Close(fd);
                    return 0;
                }
                prg_header->p_offset -= read;
            }

            while(prg_header->p_filez > 0) {
                uint32_t should_read = MIN(prg_header->p_filez, MEMORY_LOAD_SIZE);
                read = FAT_Read(part, fd, should_read, load_buf);
                if (read != should_read) {
                    printf("ELF load program Erro : read %x should read %x\n", read, should_read);
                    FAT_Close(fd);
                    return 0;
                }
                prg_header->p_filez -= read;
                memcpy(virt_addr, load_buf, read);
                virt_addr += read;
            }

            FAT_Close(fd);
        }
    }
    return 1;
}
