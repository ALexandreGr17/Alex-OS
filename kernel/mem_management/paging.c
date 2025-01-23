#include <stdint.h>

#define KERNEL_VIRT_BASE 0xC0000000 // Virtual base of the kernel
#define KERNEL_PHYS_BASE 0x00100000 // Physical address where kernel is loaded

extern void __attribute__((cdecl)) i686_load_page_dir(unsigned int*);
extern void __attribute__((cdecl)) i686_enable_paging();

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_init() {
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; // flags: Supervisor Write Invalid
    }
    for(int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
    }

    uint32_t kernel_page_table_index = KERNEL_VIRT_BASE >> 22;
    page_directory[kernel_page_table_index] = ((unsigned int)first_page_table) | 3;

    for (int i = 0; i < 1024; i++){
        first_page_table[i] = (KERNEL_PHYS_BASE + (i * 0x1000)) | 3;
    }

    page_directory[0] = ((unsigned int)first_page_table) | 3;

    i686_load_page_dir(page_directory);
    i686_enable_paging();
}
