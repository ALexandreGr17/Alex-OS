#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include "vmm_types.h"

void init_vmm_higher();
void* vmm_find_free_page();
void vmm_map_page(void* virt_addr, void* phys_addr, uint64_t nb_page);
void vmm_unmap_page(void* virt_addr, void* phys_addr, uint64_t nb_page);


#endif
