#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <boot/bootparams.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define NB_PAGE(size) (size / PAGE_SIZE + (size % PAGE_SIZE == 0 ? 0 : 1))

int init_memory_management(boot_parameters_t* bootparams);
void* allocate_new_page(uint32_t address, uint32_t nb_page);
void deallocate_page(uint32_t address, uint32_t nb_page);

#endif
