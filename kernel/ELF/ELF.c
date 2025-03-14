#include "ELF.h"
#include "arch/i686/pci/pci.h"
#include "memory_management/memory_management.h"

#include <stdint.h>
#include <stdio.h>
#include <memory/memory.h>
#include <vfs/vfs.h>

int load_elf_file(const char* filename, void** entry_point) {
    char magic[] = { 0x7F, 'E', 'L', 'F' };
    int fd = open(filename, 0);
    if (fd == -1) {
        printf("file not found\n");
        return -1;
    }
    elf_hdr_t header = { 0 };
    if (read(fd, sizeof(elf_hdr_t), &header) < sizeof(elf_hdr_t)) {
        printf("Error while reading ELF file\n");
        close(fd);
        return -2;
    }
    if (!memcmp(magic, header.signature, 4)) {
        printf("Not an ELF file\n");
        close(fd);
        return -3;
    }
    if (header.bitness != ELF_BITNESS_32) {
        printf("Wring bitness\n");
        close(fd);
        return -4;
    }

    if (header.header_version != 1) {
        printf("Wrong ELF header version\n");
        close(fd);
        return -5;
    }

    if (header.elf_version != 1) {
        printf("Wrong ELF version\n");
        close(fd);
        return -6;
    }

    if (header.type != ELF_TYPE_EXECUTABLE) {
        printf("ELF file not executable\n");
        close(fd);
        return -7;
    }

    if (header.instruction_set != ELF_INSTRUCTION_SET_X86) {
        printf("Wrong Instruction set expected: %x got: %x\n", ELF_INSTRUCTION_SET_X86, header.instruction_set);
        close(fd);
        return -8;
    }

    printf("entry point: %x\n", header.prg_entry_offset);
    *entry_point = (void*)header.prg_entry_offset;

    seek(fd, header.prg_header_table_offset, SEEK_SET);
    uint32_t prg_header_table_size = header.prg_header_table_entry_size * header.prg_header_table_entry_count;


    uint8_t* prg_header_table = calloc(prg_header_table_size, 1);

    if (read(fd, prg_header_table_size, prg_header_table) < prg_header_table_size) {
        printf("Error while reading Program header table\n");
        close(fd);
        return -2;
    }
    for (uint16_t i = 0; i < header.prg_header_table_entry_count; i++) {
        elf_pgr_hdr_entry_t* entry = (elf_pgr_hdr_entry_t*)(prg_header_table + i * header.prg_header_table_entry_size);
        if (entry->type == ELF_PROGRAM_TYPE_LOAD) {
            void* buffer = allocate_new_page(entry->p_vaddr, NB_PAGE(entry->p_memsz));
            if (buffer == NULL) {
                for (uint16_t j = 0; j < i; j++) {
                    entry = (elf_pgr_hdr_entry_t*)(prg_header_table + i * header.prg_header_table_entry_size);
                    deallocate_page(entry->p_vaddr, NB_PAGE(entry->p_memsz));
                }
                return -9;
            }
            seek(fd, entry->p_offset, SEEK_SET);
            read(fd, entry->p_filez, buffer);
        }
    }
    close(fd);
    return 0;
}
