#ifndef  PMM_H
#define PMM_H

#include <stdint.h>
#include "pmm_types.h"

void pmm_free_region(uint64_t base_addr, uint64_t length);
void pmm_alloc_region(uint64_t base_addr, uint64_t length);
void* pmm_find_free_block(uint64_t nb_blocks);
void init_pmm_higher(uint64_t* pmm_map, uint64_t size);

#endif
