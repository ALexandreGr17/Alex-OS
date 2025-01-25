#include <stdint.h>
#include <stdio.h>


#include "physical_memory_manager.h"

static uint32_t* memory_map = NULL;;
static uint32_t max_blocks = 0;
static uint32_t used_blocks = 0;

void print_hex(uint32_t b) {
    int i = 0;
    do {
        printf("%x", b%2);
        b /= 2;
        i++;
    }while(i < 32);
}

#define MAX(x, y) (x > y ? x : y)

void debug_memorymap(int min, int max) {
    printf("\n#### MEMORY MAP ####\n");
    printf("%d, %d\n", min, max);
    for (int i = MAX(0, min); i < max; i++) {
        printf("%d: ", i);
        print_hex(memory_map[i]);
        printf("\n");
    }
}


void set_block(uint32_t bit) {
    // Divide bit by 32 to get 32bit chunk of memory containing bit to be set
    // Shift 1 by remainder of bit divided by 32 to get bit ti set within the 32 bit chunk
    memory_map[bit / 32] |= (1 << (bit % 32));
}

void unset_block(uint32_t bit) {
    // Divide bit by 32 to get 32bit chunk of memory containing bit to be set
    // Shift 1 by remainder of bit divided by 32 to get bit ti set within the 32 bit chunk
    memory_map[bit / 32] &= ~(1 << (bit % 32));
}

int test_block(uint32_t bit) {
    // Divide bit by 32 to get 32bit chunk of memory containing bit to be set
    // Shift 1 by remainder of bit divided by 32 to get bit ti set within the 32 bit chunk
    return (memory_map[bit / 32] & (1 << (bit % 32)));
}

int32_t find_first_free_blocks(uint32_t num_blocks) {
    if (num_blocks == 0) {
        return -1;
    }

    for (uint32_t i = 0; i < max_blocks / 32; i++) {
        if (memory_map[i] != 0xFFFFFFFF) {
            // At least on bit is not set in this 32bit chunk of memory
            for (int32_t j = 0; j < 32; j++) {
                int32_t bit = 1 << j;
                if (!(memory_map[i] & j)) {
                    int32_t start_bit = i * 32 + j; // Get bit index i within memory map
                    uint32_t free_blocks = 0;
                    
                    // Finding num_blocks block free
                    for (uint32_t count = 0; count <= num_blocks; count++) {
                        if (!test_block(start_bit + count)) {
                            free_blocks++;
                        }
                        else {
                            break;
                        }
                        if (free_blocks == num_blocks) {
                            return i * 32 + j;
                        }
                    }
                }
            }
        }
    }
    return -1; // Not enough memory
}

void init_physical_memory_manager(uint32_t start_address, uint32_t size){
    memory_map = (uint32_t*)start_address;
    max_blocks = size / BLOCK_SIZE;
    used_blocks = max_blocks;

    // By default, set all memory in use (used blocks/bit = 1, every block is set)
    // Each byte of memory map holds 8 bits/blocks
    memset(memory_map, 0xFF, max_blocks / BLOCK_PER_BYTE);
}

void init_physical_memory_region(uint32_t base_address, uint32_t size) {
    uint32_t align = base_address / BLOCK_SIZE; // Convert mem addr to blocks
    uint32_t num_blocks = size / BLOCK_SIZE;
    for(; num_blocks > 0; num_blocks--) {
        unset_block(align++);
        used_blocks--;
    }

    set_block(0); // Protect the first block
}

void delete_physical_memory_region(uint32_t base_address, uint32_t size) {
    uint32_t align = base_address / BLOCK_SIZE; // Convert mem addr to blocks
    uint32_t num_blocks = size / BLOCK_SIZE;
    for(; num_blocks > 0; num_blocks--) {
        set_block(align++);
        used_blocks--;
    }
}

uint32_t* allocate_blocks(uint32_t num_blocks) {
    if ((max_blocks - used_blocks) <= num_blocks) {
        return NULL; // Not enough memory
    }
    int32_t starting_block = find_first_free_blocks(num_blocks);

    if (starting_block < 0) {
        return NULL;
    }

    for (uint32_t i = 0; i < num_blocks; i++) {
        set_block(starting_block + i);
    }
    used_blocks += num_blocks;
    uint32_t addr = starting_block * BLOCK_SIZE;
    return (uint32_t*)addr;
}

void free_blocks(uint32_t *address, uint32_t num_blocks) {
    uint32_t starting_block = (uint32_t)address / BLOCK_SIZE;
    for (uint32_t i = 0; i < num_blocks; i++) {
        unset_block(starting_block + i);
    }

    used_blocks -= num_blocks;
}

void print_physical_mem_info() {
    printf("\n\n");
    printf("############## PHYSICAL MEMORY INFO ##############\n");
    printf("memory size: 0x%x\n", max_blocks * BLOCK_SIZE);
    printf("memory_map location: 0x%x\n", memory_map);
    printf("max_blocks: %d\n", max_blocks);
    printf("used_blocks: %d\n", used_blocks);
    printf("\n\n");
}
