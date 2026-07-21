#include "logs/log.h"
#include <stdint.h>
#include <memory/physical/pmm.h>
#include <memory/virtual/vmm.h>

#include <boot/bootstrap.h>



void kernel_main(bootstrap_info_t* bs_info) {
    clrscr();
    // puts("test\n\t");
    logf("Hello %s%c %i %x\n", "world", '!', 10, 10);

    logf("multiboot_struct: %x\n", bs_info->multiboot_struct);

    bootstrap_info_t* info = PHYS_TO_VIRT(bs_info);
    logf("pmm_map[0]: %x\n", ((uint64_t*)PHYS_TO_VIRT(info->pmm_map))[0]);
    init_pmm_higher(PHYS_TO_VIRT(info->pmm_map), info->pmm_map_size);
    init_vmm_higher();


    uint8_t* block = pmm_find_free_block(1);
    pmm_alloc_region((uint64_t)block, 4096);
    uint8_t* page = vmm_find_free_page();

    vmm_map_page(page, block, 1);

    page[0] = 0;
    page[0] = 1;

    logf("%x: %d\n", page, page[0]);

    while(1);
}
