#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
}__attribute__((packed));

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

char* get_tag_type(uint32_t type) {
    switch (type) {
        case 1:  return "Boot command line";
        case 2:  return "Boot loader name";
        case 3:  return "Modules";
        case 4:  return "Basic memory information";
        case 5:  return "BIOS boot device";
        case 6:  return "Memory Map";
        case 7:  return "VBE info";
        case 8:  return "Framebuffer info";
        case 9:  return "ELF symbols";
        case 10:  return "APM tables";
        case 11:  return "EFI 32bit system table pointer";
        case 12:  return "EFI 64bit system table pointer";
        case 13:  return "SMBIOS tables";
        case 14:  return "ACPI old RSDP";
        case 15:  return "ACPI new RSDP";
        case 16:  return "Networking configuration";
        case 17:  return "EFI memory map";
        case 18:  return "EFI boot services not terminated";
        case 19:  return "EFI 32bit image handle pointer";
        case 20:  return "EFI 64bit image handle pointer";
        case 21:  return "Image load base physical address";
        default: return "Unknown tag";
    }

}

char* mem_region_type(uint32_t type) {
    switch (type) {
        case 1: return "Available RAM";
        case 3: return "Usable holding ACPI info";
        case 4: return "Reserved and preserved on hibernation";
        case 5: return "Defective RAM";
        default: return "Reserved";
    }
}

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

struct mem_region {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
}__attribute__((packed));

void parse_mem_region(uint8_t* buffer, size_t buffer_size) {
    size_t max_region = buffer_size / sizeof(struct mem_region);
    for (size_t i = 0; i < max_region; i++) {
        printf("\tRegion %zu:\n", i);
        struct mem_region* region = (struct mem_region*)buffer;
        printf("\t\tbase address: 0x%zx\n", region->base_addr);
        printf("\t\tlength: %zu\n", region->length);
        printf("\t\ttype: %s(%d)\n", mem_region_type(region->type), region->type);
        buffer += sizeof(struct mem_region);
    }
}

int main(void) {
    FILE* file = fopen("./dump.hex", "r");

    struct multiboot_info info = { 0 };

    ssize_t offset = 0;
    offset = fread(&info, 1, sizeof(info), file);

    printf("total_size: %i\n", info.total_size);
    printf("reserved: %i\n", info.reserved);
    struct multiboot_tag tag = { 0 };

    while (1) {
        ssize_t size = fread(&tag, 1, sizeof(tag), file);
        if (size == 0) {
            break;
        }

        printf("tag: %s(%i)\n", get_tag_type(tag.type), tag.type);
        printf("\tsize: %i\n", tag.size);
        printf("\toffset: %zu\n", offset);

        uint32_t real_size = ALIGN_UP(tag.size, 8);

        uint8_t* buffer = calloc(1, real_size - 8);
        fread(buffer, real_size - 8, 1, file);

        if (tag.type == 4) {
            printf("\tlower memory: %d\n", *(uint32_t*)buffer);
            printf("\tupper memory: %d\n", *(uint32_t*)(buffer + 4));
        }

        if (tag.type == 6) {
            printf("\tentry size: %d\n", *(uint32_t*)buffer);
            printf("\tentry version: %d\n", *(uint32_t*)(buffer + 4));
            parse_mem_region(buffer + 8, tag.size - 16);
        }

        free(buffer);
        offset += real_size;
    }


    return 0;
}
