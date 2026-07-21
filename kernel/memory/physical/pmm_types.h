#ifndef PMM_TYPES_H
#define PMM_TYPES_H

#include <stdint.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (uint64_t)4096
#endif

#define FRAME_PER_BLOCK (uint64_t)64
#define NB_BLOCK(total_memory) ((total_memory / PAGE_SIZE) / FRAME_PER_BLOCK)

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define FIND_BIT(addr) (((addr) / (PAGE_SIZE)) % (FRAME_PER_BLOCK))
#define FIND_BLOCK(addr) (((addr) / (PAGE_SIZE)) / (FRAME_PER_BLOCK))

#define GET_ADDR(block, bit) ((block) * (PAGE_SIZE) * (FRAME_PER_BLOCK) + (bit) * (PAGE_SIZE))

typedef struct multiboot_header_s {
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) multiboot_header_t;

typedef struct multiboot_tag {
    uint32_t tag;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;


struct multiboot_mem_region {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
}__attribute__((packed));


#endif
