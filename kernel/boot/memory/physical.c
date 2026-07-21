#include <stdint.h>

#include <boot/log/logf.h>
#include <memory/physical/pmm_types.h>

extern char kernel_phys_start;
extern char kernel_phys_end;

extern char __kernel_lma;
const uint64_t bootstrap_start = 0x00100000;

static uint64_t* __attribute__((section(".bootstrap_bss"))) BLOCK_BUFFER;
static uint64_t  __attribute__((section(".bootstrap_bss"))) BLOCK_BUFFER_SIZE;


static char __attribute__((section(".bootstrap_rodata"))) fmt1[] = "kernel_start: 0x%x | 0x%x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt2[] = "kernel_end: 0x%x | 0x%x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt3[] = "base: 0x%x, end: 0x%x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt4[] = "multiboot_structure: 0x%x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt5[] = "multiboot_structure_end: 0x%x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt6[] = "Not enough memory for Physical memory manager\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt7[] = "BLOCK_BUFFER : 0x%x, size: %d\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt8[] = "bootstrap start: %x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt9[] = "bootstrap end: %x\n";
static char __attribute__((section(".bootstrap_rodata"))) fmt10[] = "total_memory: %x\n";

static void* __attribute__((section(".bootstrap"))) memset(void* dest, uint8_t val, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((uint8_t*)dest)[i] = val;
    }

    return dest;
}


static uint64_t __attribute__((section(".bootstrap"))) get_mem_size_from_region(multiboot_tag_t* mem_map) {
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

static void  __attribute__((section(".bootstrap"))) pmm_free_region(uint64_t base_addr, uint64_t length) {
    for (uint64_t i = 0; i < length; i += PAGE_SIZE) {
        uint64_t bitmask = (uint64_t)1 << FIND_BIT(base_addr + i);
        uint64_t block = FIND_BLOCK((base_addr + i));
        BLOCK_BUFFER[block] &= ~bitmask;
    }
}

void  __attribute__((section(".bootstrap"))) bootstrap_pmm_alloc_region(uint64_t base_addr, uint64_t length) {
    for (uint64_t i = 0; i < length; i += PAGE_SIZE) {
        uint64_t bitmask = (uint64_t)1 << FIND_BIT(base_addr + i);
        uint64_t block = FIND_BLOCK(base_addr + i);
        BLOCK_BUFFER[block] |= bitmask;
    }
}

static void  __attribute__((section(".bootstrap"))) free_available_region(multiboot_tag_t* mem_map) {
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

static uint8_t __attribute__((section(".bootstrap"))) is_overlaping(uint64_t base, uint64_t len, uint64_t overlap_start, uint64_t overlap_end) {
    return (base >= overlap_start && base <= overlap_end) || (base + len >= overlap_start && base + len <= overlap_end) || (base <= overlap_start && base + len >= overlap_end);
}

static uint64_t* __attribute__((section(".bootstrap"))) find_free_space_for_pmm(multiboot_tag_t* mem_map, uint64_t pmm_size, uint64_t multiboot_structure, uint64_t multiboot_structure_end) {
    uint64_t buffer_size = mem_map->size - 16;
    uint8_t* buffer = (uint8_t*)mem_map + 16;

    bootstrap_logf(fmt1, kernel_phys_start, &kernel_phys_start);
    bootstrap_logf(fmt2, kernel_phys_end, &kernel_phys_end);
    bootstrap_logf(fmt8, bootstrap_start);
    bootstrap_logf(fmt9, &__kernel_lma);

    uint64_t end_bootstrap = (uint64_t)&__kernel_lma;

    uint64_t base_kernel = (uint64_t)&kernel_phys_start;
    uint64_t end_kernel = (uint64_t)&kernel_phys_end;

    uint32_t max_region = buffer_size / sizeof(struct multiboot_mem_region);
    uint64_t pmm_addr = 0;
    for (uint32_t i = 0; i < max_region; i++) {
        struct multiboot_mem_region* region = (struct multiboot_mem_region*)buffer;
        uint64_t end_address = region->base_addr + region->length;
        buffer += sizeof(struct multiboot_mem_region);
        bootstrap_logf(fmt3, region->base_addr, end_address);
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
                else if (is_overlaping(pmm_addr, pmm_size, bootstrap_start, end_bootstrap)) {
                    pmm_addr += end_bootstrap - bootstrap_start;
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

static multiboot_tag_t* __attribute__((section(".bootstrap"))) find_memory_map_in_multiboot(void* multiboot_struct) {
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

uint64_t* __attribute__((section(".bootstrap"))) init_pmm(void* multiboot_structure, uint64_t* size) {
    bootstrap_logf(fmt4, multiboot_structure);
    bootstrap_logf(fmt5, multiboot_structure + *(uint32_t*)multiboot_structure);

    uint64_t multiboot_structure_end = (uint64_t)(multiboot_structure + *(uint32_t*)multiboot_structure);

    multiboot_tag_t* memory_map = find_memory_map_in_multiboot(multiboot_structure);
    uint64_t total_memory = get_mem_size_from_region(memory_map);
    bootstrap_logf(fmt10, total_memory);
    BLOCK_BUFFER = find_free_space_for_pmm(memory_map, NB_BLOCK(total_memory) * 8, (uint64_t)multiboot_structure, multiboot_structure_end);
    if (!BLOCK_BUFFER) {
        bootstrap_logf(fmt6);
        return 0;
    }

    BLOCK_BUFFER_SIZE = NB_BLOCK(total_memory);
    bootstrap_logf(fmt7, BLOCK_BUFFER, BLOCK_BUFFER_SIZE * 8);

    memset((uint8_t*)BLOCK_BUFFER, 0xFF, BLOCK_BUFFER_SIZE * 8);
    free_available_region(memory_map);
    bootstrap_pmm_alloc_region((uint64_t)&kernel_phys_start, &kernel_phys_end - &kernel_phys_start);
    bootstrap_pmm_alloc_region(bootstrap_start, (uint64_t)&__kernel_lma - bootstrap_start);
    bootstrap_pmm_alloc_region((uint64_t)multiboot_structure, multiboot_structure_end - (uint64_t)multiboot_structure);
    bootstrap_pmm_alloc_region((uint64_t)BLOCK_BUFFER, BLOCK_BUFFER_SIZE * 8);
    return BLOCK_BUFFER;
}

void* __attribute__((section(".bootstrap"))) bootstrap_pmm_find_free_block(uint64_t nb_blocks) {
    uint64_t count = nb_blocks;
    void* base = 0;
    for (uint64_t i = 0; i < BLOCK_BUFFER_SIZE; i++) {
        if (BLOCK_BUFFER[i] == (uint64_t)-1) {
            continue;
        }

        uint64_t shift = 0;
        while (shift < 64) {
            if (!(BLOCK_BUFFER[i] & (1 << shift))) {
                if (count == nb_blocks) {
                    base = (void*)GET_ADDR(i, shift);
                }
                count--;
                if (count == 0) {
                    return base;
                }
            }
            else {
                count = nb_blocks;
                base = 0;
            }
            shift++;
        }
    }

    return 0;
}
