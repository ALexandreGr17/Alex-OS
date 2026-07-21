#include <stdint.h>

#include <logs/log.h>
#include <memory/mem_utils.h>
#include "pmm_types.h"

extern char kernel_start;
extern char kernel_end;

static uint64_t* BLOCK_BUFFER;
static uint64_t BLOCK_BUFFER_SIZE;

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

void* pmm_find_free_block(uint64_t nb_blocks) {
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

void init_pmm_higher(uint64_t* pmm_map, uint64_t size) {
    BLOCK_BUFFER = pmm_map;
    BLOCK_BUFFER_SIZE = size;
}
