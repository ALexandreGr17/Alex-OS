#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void init_pmm(void* multiboot_structure);
void init_vmm_lower(void (*entry)());
void* vmm_map_page(void* virt_addr, void* phys_addr, uint64_t nb_page);

#endif
