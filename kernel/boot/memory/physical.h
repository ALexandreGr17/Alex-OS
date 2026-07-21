#ifndef PHYSICAL_H
#define PHYSICAL_H

#include <stdint.h>

uint64_t* init_pmm(void* multiboot_structure, uint64_t* map_size);
void* bootstrap_pmm_find_free_block(uint64_t nb_blocks);
void bootstrap_pmm_alloc_region(uint64_t base_addr, uint64_t length);

#endif
