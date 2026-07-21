#include <stdint.h>
#include <memory/physical/pmm.h>
#include <memory/mem_utils.h>
#include <memory/virtual/vmm_types.h>

#include <logs/log.h>

static uint64_t* PML4;

extern void* get_cr3();
extern void set_cr3(void*);

void* vmm_find_free_page() {
    for (uint16_t i = 0; i < 512; i++) {
 
        if (!(PML4[i] & PAGE_ATTRIBUTE_PRESENT)) return (void*)GET_VIRT(i, 0, 0, 0, 0);
        uint64_t *pdpt_table = PHYS_TO_VIRT((void *)(PML4[i] & PAGE_ADDRESS_MASK));

        for (uint16_t j = 0; j < 512; j++) {
            if (!((uint64_t)pdpt_table[j] & PAGE_ATTRIBUTE_PRESENT)) return (void*)GET_VIRT(i, j, 0, 0, 0);
            uint64_t *pd_table = PHYS_TO_VIRT((void *)(pdpt_table[j] & PAGE_ADDRESS_MASK));

            for (uint16_t k = 0; k < 512; k++) { 
                if (!((uint64_t)pd_table[k] & PAGE_ATTRIBUTE_PRESENT)) return (void*)GET_VIRT(i, j, k, 0, 0);
                if (((uint64_t)pd_table[k] & PAGE_ATTRIBUTE_SIZE)) continue;
                uint64_t *pt_table = PHYS_TO_VIRT((void *)(pd_table[k] & PAGE_ADDRESS_MASK));
                
                for (uint16_t l = 0; l < 512; l++) { 
                    if (!((uint64_t)pt_table[l] & PAGE_ATTRIBUTE_PRESENT)) return (void*)GET_VIRT(i, j, k, l, 0);
                }

            }
        }
    }
    return (void*)0;
}

void vmm_map_page(void* virt_addr, void* phys_addr, uint64_t nb_page) {
    while (nb_page > 0) {
        if (!((uint64_t)PML4[GET_PML4(virt_addr)] & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            PML4[GET_PML4(virt_addr)] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pdpt_table = PHYS_TO_VIRT(PML4[GET_PML4(virt_addr)] & PAGE_ADDRESS_MASK);

        uint64_t entry = pdpt_table[GET_PDPT(virt_addr)];

        if (!(entry & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pdpt_table[GET_PDPT(virt_addr)] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pd_table = PHYS_TO_VIRT(pdpt_table[GET_PDPT(virt_addr)] & PAGE_ADDRESS_MASK);

        entry = pd_table[GET_PD(virt_addr)];


        if (!(entry & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pd_table[GET_PD(virt_addr)] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pt_table = PHYS_TO_VIRT(pd_table[GET_PD(virt_addr)] & PAGE_ADDRESS_MASK);
        if (pt_table[GET_PT(virt_addr)] & PAGE_ATTRIBUTE_PRESENT) return;
        pt_table[GET_PT(virt_addr)] = ((uint64_t)phys_addr & ~(0xFFF)) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
        nb_page--;
    }

}


void vmm_unmap_page(void* virt_addr, void* phys_addr, uint64_t nb_page) {
    while (nb_page > 0) {
        if (!(PML4[GET_PML4(virt_addr)] & PAGE_ATTRIBUTE_PRESENT)) return;
        uint64_t *pdpt_table = PHYS_TO_VIRT(PML4[GET_PML4(virt_addr)] & PAGE_ADDRESS_MASK);

        if (!(pdpt_table[GET_PDPT(virt_addr)] & PAGE_ATTRIBUTE_PRESENT)) return;
        uint64_t *pd_table = PHYS_TO_VIRT(pdpt_table[GET_PDPT(virt_addr)] & PAGE_ADDRESS_MASK);


        if (!(pd_table[GET_PD(virt_addr)] & PAGE_ATTRIBUTE_PRESENT)) return;
        uint64_t *pt_table = PHYS_TO_VIRT(pd_table[GET_PD(virt_addr)] & PAGE_ADDRESS_MASK);

        pt_table[GET_PT(virt_addr)] = 0;
        nb_page--;
    }
}

void init_vmm_higher() {
    PML4 = PHYS_TO_VIRT(get_cr3());
}
