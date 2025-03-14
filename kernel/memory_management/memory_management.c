#include "memory_management.h"

#include <boot/bootparams.h>
#include <stdint.h>
#include <stdio.h>

#include "virtual/virtual_memory_manager.h"
#include "physique/physical_memory_manager.h"


#define MEMORY_MAP_LOCATION 0x30000
#define ARBITRARY_KERNEL_SIZE 0x9000

int init_memory_management(boot_parameters_t* bootparams) {

    memory_region_t* last = &bootparams->Memory.regions[bootparams->Memory.region_count - 1];
    uint32_t total_memory = last->Begin + last->Length - 1;

    init_physical_memory_manager(MEMORY_MAP_LOCATION, total_memory);

    // init memory region fot the Available memory region
    for (uint32_t i = 0; i < bootparams->Memory.region_count; i++) {
        if (bootparams->Memory.regions[i].Type == 1) {
            init_physical_memory_region(bootparams->Memory.regions[i].Begin, bootparams->Memory.regions[i].Length);
        }
    }

    // Set certain regions/blocks as used or reserved
    printf("krenel location: 0x%x\n", bootparams->kernel_location);
    delete_physical_memory_region(bootparams->kernel_location, ARBITRARY_KERNEL_SIZE);
    delete_physical_memory_region(0, 0x1000);

    print_physical_mem_info();

    printf("Initialize paging\n");
    int i = init_virtual_memory_manager(bootparams->kernel_location);

    

    // TODO: 
    //  identity mapping
    //  Kmalloc, Kcalloc, Kfree, Krealloc (kernel version)

    return i;
}

void* allocate_new_page(uint32_t address, uint32_t nb_page) {
    void* base_address = allocate_blocks(nb_page);
    if (base_address == NULL) {
        return NULL;
    }
    void* tmp_base_address = base_address;
    void* tmp_address = (void*)address;
    for (uint32_t i = 0; i < nb_page; i++, tmp_base_address += PAGE_SIZE, tmp_address += PAGE_SIZE) {
        if (!map_page(tmp_base_address, (void*)tmp_address)) {
            free_blocks(base_address, nb_page);
            tmp_address = (void*)address;
            for (uint32_t j = 0; j < i; j++, tmp_address += PAGE_SIZE) {
                unmap_page(tmp_address);
            }
            return NULL;
        }
    }
    return (void*)address;
}

void deallocate_page(uint32_t address, uint32_t nb_page) {
    void* base_address = get_page(address);
    free_blocks(base_address, nb_page);
    for (uint32_t i = 0; i < nb_page; i++, address += PAGE_SIZE) {
        unmap_page((void*)address);
    }
}
