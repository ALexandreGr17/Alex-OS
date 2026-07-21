#ifndef VMM_TYPES_H
#define VMM_TYPES_H

#define HHDM_OFFSET 0xffff800000000000ULL

#ifndef PAGE_SIZE
#define PAGE_SIZE (4096)
#endif

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
    PAGE_ATTRIBUTE_PRESENT = 1,
    PAGE_ATTRIBUTE_WRITE = (1 << 1),
    PAGE_ATTRIBUTE_USER = (1 << 2),
    PAGE_ATTRIBUTE_WT = (1 << 3),
    PAGE_ATTRIBUTE_CD = (1 << 4),
    PAGE_ATTRIBUTE_ACCESSED = (1 << 5),
    PAGE_ATTRIBUTE_SIZE = (1 << 7),
};


#endif
