#include "boot/bootstrap.h"
#include <memory/virtual/vmm_types.h>
#include <stdint.h>
#include <boot/memory/physical.h>
#include <boot/log/logf.h>


static uint64_t  __attribute__((section(".bootstrap_bss"))) PML4[512] __attribute__((aligned(4096)));
static uint64_t __attribute__((section(".bootstrap_bss"))) PDPT[512] __attribute__((aligned(4096)));
static uint64_t __attribute__((section(".bootstrap_bss"))) PD[512] __attribute__((aligned(4096)));

extern void* bootstrap_get_cr3();
extern void bootstrap_set_cr3(void*);

extern char kernel_phys_start;
extern char kernel_phys_end;


const uint64_t __attribute__((section(".bootstrap_rodata"))) base_kernel = (uint64_t)&kernel_phys_start;
const uint64_t __attribute__((section(".bootstrap_rodata"))) end_kernel = (uint64_t)&kernel_phys_end;

static void* __attribute__((section(".bootstrap"))) memset(void* dest, uint8_t val, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((uint8_t*)dest)[i] = val;
    }

    return dest;
}

void __attribute__((section(".bootstrap"))) init_vmm(entry_point_t entry, bootstrap_info_t* info) {
    const uint64_t kernel_size = end_kernel - base_kernel;
    const uint64_t higher_half = 0xFFFFFFFF80000000;

    const uint64_t* old_pml4 = bootstrap_get_cr3();

    PML4[0] = *old_pml4;

    // PD[GET_PD(HHDM_OFFSET)] = (uint64_t)PT | PAGE_SIZE | PAGE_PRESENT | PAGE_WRITE;
    PDPT[GET_PDPT(HHDM_OFFSET)] = (uint64_t)PD | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
    PML4[GET_PML4(HHDM_OFFSET)] = (uint64_t)PDPT | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
    for (uint64_t i = 0; i < 512 ; i++) {
        PD[i] = (i * 0x200000ULL) | PAGE_ATTRIBUTE_SIZE | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
    }
    bootstrap_set_cr3(PML4);

    for (uint64_t i = 0; i < kernel_size; i+=4096) {
        if (!((uint64_t)PML4[GET_PML4((higher_half + i))] & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = bootstrap_pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            PML4[GET_PML4((higher_half + i))] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            bootstrap_pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pdpt_table = PHYS_TO_VIRT(PML4[GET_PML4((higher_half + i))] & PAGE_ADDRESS_MASK);

        uint64_t entry = pdpt_table[GET_PDPT((higher_half + i))];

        if (!(entry & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = bootstrap_pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pdpt_table[GET_PDPT((higher_half + i))] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            bootstrap_pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pd_table = PHYS_TO_VIRT(pdpt_table[GET_PDPT((higher_half + i))] & PAGE_ADDRESS_MASK);

        entry = pd_table[GET_PD((higher_half + i))];


        if (!(entry & PAGE_ATTRIBUTE_PRESENT)) {
            void* block_phys = bootstrap_pmm_find_free_block(1);
            uint64_t *table = PHYS_TO_VIRT(block_phys);
            memset(table, 0, 4096);
            pd_table[GET_PD((higher_half + i))] = (uint64_t)(block_phys) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
            bootstrap_pmm_alloc_region((uint64_t)block_phys, 4096);
        }
        uint64_t *pt_table = PHYS_TO_VIRT(pd_table[GET_PD((higher_half + i))] & PAGE_ADDRESS_MASK);
        if (pt_table[GET_PT((higher_half + i))] & PAGE_ATTRIBUTE_PRESENT) return;
        pt_table[GET_PT((higher_half + i))] = ((uint64_t)(base_kernel + i) & ~(0xFFF)) | PAGE_ATTRIBUTE_PRESENT | PAGE_ATTRIBUTE_WRITE;
    }
    entry(info);
}
