#include "ELF.h"

#include <stdio.h>
#include <memory/memory.h>
#include <vfs/vfs.h>

int load_elf_file(const char* filename) {
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

    if (header.type != ELF_INSTRUCTION_SET_X86) {
        printf("Wrong Instruction set\n");
        close(fd);
        return -8;
    }

    return 0;
}
