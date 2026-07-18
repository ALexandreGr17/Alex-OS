#include <stdint.h>
#include <memory/physical/physical_memory_manager.h>
#include <memory/mem_utils.h>

#include <logs/log.h>

static uint64_t PML4[512] __attribute__((aligned(4096)));
static uint64_t PDPT[512] __attribute__((aligned(4096)));
static uint64_t PD[512] __attribute__((aligned(4096)));

#define HHDM_OFFSET 0xffff800000000000ULL

#define SET_ATRRIBUTE(entry, flags) ((entry) |= (flags))

#define GET_PML4(virt_addr) ((((uint64_t)virt_addr) >> 39) & 0x1FF)
#define GET_PDPT(virt_addr) ((((uint64_t)virt_addr) >> 30) & 0x1FF)
#define GET_PD(virt_addr) ((((uint64_t)virt_addr) >> 21) & 0x1FF)
#define GET_PT(virt_addr) ((((uint64_t)virt_addr) >> 12) & 0x1FF)
#define GET_VIRT(pml4_i, pdpt_i, pd_i, pt_i, off) ((uint64_t) ((uint64_t)pml4_i << 39) | ((uint64_t)pdpt_i << 30) | ((uint64_t)pd_i << 21) | ((uint64_t)pt_i << 12) | off )
#define PHYS_TO_VIRT(p) ((void*)((uint64_t)(p) + HHDM_OFFSET))
#define VIRT_TO_PHYS(v) ((uint64_t)(v) - HHDM_OFFSET)
#define PAGE_ADDRESS_MASK ~0xFFFULL

enum PAGE_ATTRIBUTE {
    PAGE_PRESENT = 1,
    PAGE_WRITE = (1 << 1),
    PAGE_USER = (1 << 2),
    PAGE_WT = (1 << 3),
    PAGE_CD = (1 << 4),
    PAGE_ACCESSED = (1 << 5),
    PAGE_SIZE = (1 << 7),
};

extern void* get_cr3();
extern void set_cr3(void*);

extern char kernel_start;
extern char kernel_end;

void kernel_loop();

const uint64_t base_kernel = (uint64_t)&kernel_start;
const uint64_t end_kernel = (uint64_t)&kernel_end;

void* find_free_page() {
    for (uint16_t i = 0; i < 512; i++) {
 
        if (!(PML4[i] & PAGE_PRESENT)) continue;
        uint64_t *pdpt_table = PHYS_TO_VIRT((void *)(PML4[i] & PAGE_ADDRESS_MASK));

        for (uint16_t j = 0; j < 512; j++) {
            if (!((uint64_t)pdpt_table[j] & PAGE_PRESENT)) continue;
            uint64_t *pd_table = PHYS_TO_VIRT((void *)(pdpt_table[j] & PAGE_ADDRESS_MASK));

            for (uint16_t k = 0; k < 512; k++) { 
                if (!((uint64_t)pd_table[k] & PAGE_PRESENT)) continue;
                uint64_t *pt_table = PHYS_TO_VIRT((void *)(pd_table[k] & PAGE_ADDRESS_MASK));
                
                for (uint16_t l = 0; l < 512; l++) { 
                    if (!((uint64_t)pt_table[l] & PAGE_PRESENT)) return (void*)GET_VIRT(i, j, k, l, 0);
                }

            }
        }
    }
    return (void*)0;
}

void vmm_map_page(void* virt_addr, void* phys_addr, uint64_t nb_page) {
    while (nb_page > 0) {
        if (!((uint64_t)PML4[GET_PML4(virt_addr)] & PAGE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            PML4[GET_PML4(virt_addr)] = (uint64_t)(block_phys) | PAGE_PRESENT | PAGE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pdpt_table = PHYS_TO_VIRT(PML4[GET_PML4(virt_addr)] & PAGE_ADDRESS_MASK);
        logf("pdpt_table(%x): %x\n", virt_addr, pdpt_table);

        uint64_t entry = pdpt_table[GET_PDPT(virt_addr)];

        if (!(entry & PAGE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pdpt_table[GET_PDPT(virt_addr)] = (uint64_t)(block_phys) | PAGE_PRESENT | PAGE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pd_table = PHYS_TO_VIRT(pdpt_table[GET_PDPT(virt_addr)] & PAGE_ADDRESS_MASK);
        logf("pd_table(%x): %x\n", virt_addr, pd_table);

        entry = pd_table[GET_PD(virt_addr)];


        if (!(entry & PAGE_PRESENT)) {
            void* block_phys = pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pd_table[GET_PD(virt_addr)] = (uint64_t)(block_phys) | PAGE_PRESENT | PAGE_WRITE;
            pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pt_table = PHYS_TO_VIRT(pd_table[GET_PD(virt_addr)] & PAGE_ADDRESS_MASK);
        if (pt_table[GET_PT(virt_addr)] & PAGE_PRESENT) return;
        pt_table[GET_PT(virt_addr)] = ((uint64_t)phys_addr & ~(0xFFF)) | PAGE_PRESENT | PAGE_WRITE;
        nb_page--;
    }

}


void vmm_unmap_page(void* virt_addr, void* phys_addr, uint64_t nb_page) {
    while (nb_page > 0) {
        if (!(PML4[GET_PML4(virt_addr)] & PAGE_PRESENT)) return;
        uint64_t *pdpt_table = PHYS_TO_VIRT(PML4[GET_PML4(virt_addr)] & PAGE_ADDRESS_MASK);

        if (!(pdpt_table[GET_PDPT(virt_addr)] & PAGE_PRESENT)) return;
        uint64_t *pd_table = PHYS_TO_VIRT(pdpt_table[GET_PDPT(virt_addr)] & PAGE_ADDRESS_MASK);


        if (!(pd_table[GET_PD(virt_addr)] & PAGE_PRESENT)) return;
        uint64_t *pt_table = PHYS_TO_VIRT(pd_table[GET_PD(virt_addr)] & PAGE_ADDRESS_MASK);

        pt_table[GET_PT(virt_addr)] = 0;
        nb_page--;
    }
}

void init_vmm_higher(void (*entry)()) {
    // PML4[0] = 0;
    logf("success, entry point: ");
    logf("%x\n", entry);
    logf("%x\n", &kernel_start);
    // entry();
}

void init_vmm_lower(void (*entry)()) {
    const uint64_t kernel_size = end_kernel - base_kernel;
    const uint64_t higher_half = 0xFFFFFFFF80000000;

    // if (kernel_size / 4096  >= 512) {
    //     // temporary
    //     logf("Kernel too big");
    //     for(;;);
    // }

    const uint64_t* old_pml4 = (get_cr3());

    PML4[0] = *old_pml4;

    // PD[GET_PD(HHDM_OFFSET)] = (uint64_t)PT | PAGE_SIZE | PAGE_PRESENT | PAGE_WRITE;
    PDPT[GET_PDPT(HHDM_OFFSET)] = (uint64_t)PD | PAGE_PRESENT | PAGE_WRITE;
    PML4[GET_PML4(HHDM_OFFSET)] = (uint64_t)PDPT | PAGE_PRESENT | PAGE_WRITE;
    for (uint64_t i = 0; i < 512 ; i++) {
        PD[i] = (i * 0x200000ULL) | PAGE_SIZE | PAGE_PRESENT | PAGE_WRITE;
    }

    logf("switch page\n");
    set_cr3(PML4);
    logf("All good\n");

    for (uint64_t i = 0; i < kernel_size; i+=4096) {
       vmm_map_page((void*)(higher_half + i), (void*)(base_kernel + i), 1);
    }

    void (*kernel_high)(void (*entry)()) = (void*)higher_half + ((uint64_t)init_vmm_higher - base_kernel);
    kernel_high((void*)higher_half + ((uint64_t)entry - base_kernel));
}
