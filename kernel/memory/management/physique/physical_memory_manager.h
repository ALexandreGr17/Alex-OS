#ifndef PHYSICAL_MEMORY_MANAGER_H
#define PHYSICAL_MEMORY_MANAGER_H

#include <stdint.h>
#include <memory/memory.h>

#define BLOCK_SIZE 4096
#define BLOCK_PER_BYTE 8

void set_block(uint32_t bit);
void unset_block(uint32_t bit);
int test_block(uint32_t bit);
int32_t find_first_free_blocks(uint32_t num_blocks);
void init_physical_memory_manager(uint32_t start_address, uint32_t size);
void init_physical_memory_region(uint32_t base_address, uint32_t size);
void delete_physical_memory_region(uint32_t base_address, uint32_t size);
uint32_t* allocate_blocks(uint32_t num_blocks);
void free_blocks(uint32_t *address, uint32_t num_blocks);
void print_physical_mem_info();

#endif
