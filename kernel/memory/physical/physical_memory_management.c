#include <stdint.h>

#include <logs/log.h>

extern char kernel_start;
extern char kernel_end;

#ifndef PAGE_SIZE
#define PAGE_SIZE (uint64_t)4096
#endif

#define FRAME_PER_BLOCK (uint64_t)64
#define NB_BLOCK(total_memory) ((total_memory / PAGE_SIZE) / FRAME_PER_BLOCK)

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define FIND_BIT(addr) (((addr) / (PAGE_SIZE)) % (FRAME_PER_BLOCK))
#define FIND_BLOCK(addr) (((addr) / (PAGE_SIZE)) / (FRAME_PER_BLOCK))

#define GET_ADDR(block, bit) ((block) * (PAGE_SIZE) * (FRAME_PER_BLOCK) + (bit) * (PAGE_SIZE))

typedef struct multiboot_header_s {
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) multiboot_header_t;

typedef struct multiboot_tag {
    uint32_t tag;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;


struct multiboot_mem_region {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
}__attribute__((packed));

static uint64_t* BLOCK_BUFFER;
static uint64_t BLOCK_BUFFER_SIZE;


uint64_t get_mem_size_from_region(multiboot_tag_t* mem_map) {
    uint64_t buffer_size = mem_map->size - 16;
    uint8_t* buffer = (uint8_t*)mem_map + 16;

    uint32_t max_region = buffer_size / sizeof(struct multiboot_mem_region);
    uint64_t total_memory = 0;
    for (uint32_t i = 0; i < max_region; i++) {
        struct multiboot_mem_region* region = (struct multiboot_mem_region*)buffer;
        buffer += sizeof(struct multiboot_mem_region);
        if (region->base_addr + region->length > total_memory) {
            total_memory = region->base_addr + region->length;
        }
    }
    return total_memory;
}

void pmm_free_region(uint64_t base_addr, uint64_t length) {
    for (uint64_t i = 0; i < length; i += PAGE_SIZE) {
        uint64_t bitmask = (uint64_t)1 << FIND_BIT(base_addr + i);
        uint64_t block = FIND_BLOCK((base_addr + i));
        BLOCK_BUFFER[block] &= ~bitmask;
    }
}

void pmm_alloc_region(uint64_t base_addr, uint64_t length) {
    for (uint64_t i = 0; i < length; i += PAGE_SIZE) {
        uint64_t bitmask = (uint64_t)1 << FIND_BIT(base_addr + i);
        uint64_t block = FIND_BLOCK(base_addr + i);
        BLOCK_BUFFER[block] |= bitmask;
    }
}

void free_available_region(multiboot_tag_t* mem_map) {
    uint64_t buffer_size = mem_map->size - 16;
    uint8_t* buffer = (uint8_t*)mem_map + 16;

    uint32_t max_region = buffer_size / sizeof(struct multiboot_mem_region);
    for (uint32_t i = 0; i < max_region; i++) {
        struct multiboot_mem_region* region = (struct multiboot_mem_region*)buffer;
        buffer += sizeof(struct multiboot_mem_region);
        if (region->type == 1 && region->base_addr != 0) {
            pmm_free_region(region->base_addr, region->length);
        }
    }
}

uint8_t is_overlaping(uint64_t base, uint64_t len, uint64_t overlap_start, uint64_t overlap_end) {
    return (base >= overlap_start && base <= overlap_end) || (base + len >= overlap_start && base + len <= overlap_end);
}

uint64_t* find_free_space_for_pmm(multiboot_tag_t* mem_map, uint64_t pmm_size, uint64_t multiboot_structure, uint64_t multiboot_structure_end) {
    uint64_t buffer_size = mem_map->size - 16;
    uint8_t* buffer = (uint8_t*)mem_map + 16;

    logf("kernel_start: 0x%x | 0x%x\n", kernel_start, &kernel_start);
    logf("kernel_end: 0x%x | 0x%x\n", kernel_end, &kernel_end);

    uint64_t base_kernel = (uint64_t)&kernel_start;
    uint64_t end_kernel = (uint64_t)&kernel_end;

    uint32_t max_region = buffer_size / sizeof(struct multiboot_mem_region);
    uint64_t pmm_addr = 0;
    for (uint32_t i = 0; i < max_region; i++) {
        struct multiboot_mem_region* region = (struct multiboot_mem_region*)buffer;
        uint64_t end_address = region->base_addr + region->length;
        buffer += sizeof(struct multiboot_mem_region);
        logf("base: 0x%x, end: 0x%x\n", region->base_addr, end_address);
        if (region->type == 1) {
            if (region->base_addr == 0) {
                continue;
            }
            if (end_address >= 1000000000) { // seulement 1 Go de map 
                return 0;
            }
            pmm_addr = region->base_addr;

            while (pmm_addr + pmm_size < region->base_addr + region->length) {
                if (is_overlaping(pmm_addr, pmm_size, base_kernel, end_kernel)) {
                    pmm_addr += end_kernel - base_kernel;
                }
                else if (is_overlaping(pmm_addr, pmm_size, multiboot_structure, multiboot_structure_end)) {
                    pmm_addr += multiboot_structure_end - multiboot_structure;
                }
                else {
                    break;
                }
            }

            if (pmm_addr + pmm_size < region->base_addr + region->length) {
                return (uint64_t*)pmm_addr;
            }
        }
    }
    return 0;
}

multiboot_tag_t* find_memory_map_in_multiboot(void* multiboot_struct) {
    multiboot_header_t* header = multiboot_struct;
    multiboot_struct += sizeof(multiboot_header_t);

    uint32_t offset = sizeof(multiboot_header_t);
    while (offset < header->size) {
        multiboot_tag_t* tag = multiboot_struct;
        uint32_t real_size = ALIGN_UP(tag->size, 8);
        if (tag->tag == 6) {
            return tag;
        }
        multiboot_struct += real_size;
        offset += real_size;
    }
    return 0;
}

void memset(uint8_t *s, uint8_t c, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        s[i] = c;
    }
}

void init_pmm(void* multiboot_structure) {
    logf("multiboot_structure: 0x%x\n", multiboot_structure);
    logf("multiboot_structure_end: 0x%x\n", multiboot_structure + *(uint32_t*)multiboot_structure);

    uint64_t multiboot_structure_end = (uint64_t)(multiboot_structure + *(uint32_t*)multiboot_structure);

    multiboot_tag_t* memory_map = find_memory_map_in_multiboot(multiboot_structure);
    uint64_t total_memory = get_mem_size_from_region(memory_map);
    BLOCK_BUFFER = find_free_space_for_pmm(memory_map, NB_BLOCK(total_memory) * 8, (uint64_t)multiboot_structure, multiboot_structure_end);
    if (!BLOCK_BUFFER) {
        logf("Not enough memory for Physical memory manager\n");
        return;
    }
    logf("BLOCK_BUFFER : 0x%x\n", BLOCK_BUFFER);

    BLOCK_BUFFER_SIZE = NB_BLOCK(total_memory);
    memset((uint8_t*)BLOCK_BUFFER, 0xFF, BLOCK_BUFFER_SIZE * 8);
    free_available_region(memory_map);
    pmm_alloc_region((uint64_t)&kernel_start, &kernel_end - &kernel_start);
    pmm_alloc_region((uint64_t)multiboot_structure, multiboot_structure_end - (uint64_t)multiboot_structure);
    pmm_alloc_region((uint64_t)BLOCK_BUFFER, BLOCK_BUFFER_SIZE);
}

void* pmm_find_free_block() {
    for (uint64_t i = 0; i < BLOCK_BUFFER_SIZE; i++) {
        if (BLOCK_BUFFER[i] == (uint64_t)-1) {
            continue;
        }

        uint64_t shift = 0;
        while (shift < 64) {
            if (!(BLOCK_BUFFER[i] & (1 << shift))) {
                return (void*)GET_ADDR(i, shift);
            }
        }
    }

    return 0;
}
