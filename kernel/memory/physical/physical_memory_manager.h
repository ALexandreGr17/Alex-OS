#ifndef PHYSICAL_MEMORY_MANAGER_H
#define PHYSICAL_MEMORY_MANAGER_H

#include <stdint.h>

void init_pmm(void* multiboot_structure);
void pmm_alloc_region(uint64_t base_addr, uint64_t length);
void pmm_free_region(uint64_t base_addr, uint64_t length);
void* pmm_find_free_block(uint64_t nb_blocks);

#endif
