
#include <arch/i686/isr.h>
#include <stdint.h>
#include <stdio.h>
#include <arch/i686/io.h>
#include "../physique/physical_memory_manager.h"

#include "virtual_memory_manager.h"
#include "x86_virtual_mem.h"

page_directory* current_page_dir = NULL;

struct page_directory_vec_s {
    page_directory** vec;
    uint32_t size;
    uint32_t cap;
};

struct page_directory_vec_s* page_dir_vec = NULL;

void i686_Page_fault_handler(Register* regs) {
    printf("Page Fault\n");
    printf("Bad address: 0x%x\n", i686_get_cr2());
    i686_panic();
}


uint32_t *get_page_table_entry(page_table* pt, uint32_t address){
    if (pt) {
        return &pt->entries[PT_INDEX(address)];
    }
    return NULL;
}
uint32_t *get_page_directory_entry(page_directory* pd, uint32_t address){
    if (pd) {
        return &pd->entries[PT_INDEX(address)];
    }
    return NULL;
}

uint32_t* get_page(uint32_t virtual_address) {
    page_directory* pd = current_page_dir;
    uint32_t* entry = &pd->entries[PD_INDEX(virtual_address)];
    page_table* table = (page_table*)PAGE_PHYS_ADDR(entry);
    uint32_t* page = &table->entries[PT_INDEX(virtual_address)];
    return page;
}

void* allocate_page(uint32_t *page) {
    void* block = allocate_blocks(1);
    if (block) {
        SET_FRAME(page, (uint32_t)block);
        SET_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT);
    }
    return block;
}

void free_page(uint32_t* page) {
    void* address = (void*)PAGE_PHYS_ADDR(page);
    if (address) {
        free_blocks(address, 1);
    }
    UNSET_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT);
}

uint8_t set_page_directory(page_directory* pd) {
    if (!pd) {
        return 0;
    }

    current_page_dir = pd;

    printf("Setting CR3 to: 0x%x\n", current_page_dir);
    i686_load_page_dir(current_page_dir);
    return 1;
}

void flush_tlb_entry(uint32_t virtual_address) {
    __asm__ __volatile__ ("cli; invlpg (%0); sti" : : "r"(virtual_address) );
}

uint8_t map_page(void* physical_address, void *virtual_address) {
    page_directory *pd = current_page_dir;
    uint32_t* entry = &pd->entries[PD_INDEX((uint32_t)virtual_address)];

    if (!TST_ATTRIBUTE(entry, PAGE_TABLE_ENTRY_PRESENT)) {
        // Page not present so we are allocating it
        page_table* table = (page_table*)allocate_blocks(1);
        if (!table) {
            return 0; // Out of memory
        }
        memset(table, 0, sizeof(page_table));

        uint32_t* entry = &pd->entries[PD_INDEX((uint32_t)virtual_address)];
        SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_PRESENT);
        SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_READ_WRITE);
        SET_FRAME(entry, (uint32_t)table);
    }

    page_table* table = (page_table*)PAGE_PHYS_ADDR(entry);

    uint32_t* page = &table->entries[PT_INDEX((uint32_t)virtual_address)];

    SET_FRAME(page, (uint32_t)physical_address);
    SET_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT);
    return 1;
}
//
// void* find_free_page() {
//     page_directory *pd = current_page_dir;
//     for (uint32_t i = 0; i < (3 * PAGE_SIZE) / sizeof(uint32_t); i++) {
//         uint32_t* entry = &pd->entries[i];
//         page_table* table = NULL;
//         if (!TST_ATTRIBUTE(entry, PAGE_TABLE_ENTRY_PRESENT)) {
//             // Page not present so we are allocating it
//              table = (page_table*)allocate_blocks(1);
//             if (!table) {
//                 return 0; // OOM
//             }
//             memset(table, 0, sizeof(page_table));
//         }
//         else {
//             table = (page_table*)PAGE_PHYS_ADDR(entry);
//         }
//         for (uint32_t j = 0; j < PAGE_SIZE / sizeof(uint32_t); j++) {
//             uint32_t* page = &table->entries[0];
//             if (!TST_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT)) {
//                 return page;
//             }
//         }
//     }
//     return NULL; // OOM
// }

void unmap_page(void* virtual_address) {
    uint32_t* page = get_page((uint32_t)virtual_address);
    SET_FRAME(page, 0);
    UNSET_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT);
    __asm__ volatile("invlpg (%0)" : : "r" (virtual_address) : "memory");
}

void* find_free_page(uint32_t nb_page) {
    page_directory* pd = current_page_dir;
    void* base_addr = NULL;
    uint32_t count = 0;
    for (uint32_t i = 0; i < (3 * PAGE_SIZE) / sizeof(uint32_t); i++) {
        uint32_t* entry = &pd->entries[i];
        page_table* table = NULL;
        if (!TST_ATTRIBUTE(entry, PAGE_TABLE_ENTRY_PRESENT)) {
            // Page not present sp we are allocating it
            table = (page_table*)allocate_blocks(1);
            if (!table) {
                return NULL;
            }
            memset(table, 0, sizeof(page_table));
            SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_PRESENT);
            SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_READ_WRITE);
            SET_FRAME(entry, (uint32_t)table);
        }
        else {
            table = (page_table*)PAGE_PHYS_ADDR(entry);
        }
        for (uint32_t j = 0; j < PAGE_SIZE / sizeof(uint32_t); j++) {
            uint32_t* page = &table->entries[j];
            if (!TST_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT)) {
                if (base_addr == NULL) {
                    base_addr = (void*)((i << 22) | (j << 12));
                }

                if (count == nb_page) {
                    return base_addr;
                }
                count++;
            }
            else {
                base_addr = NULL;
                count = 0;
            }
        }
    }
    return NULL;
}

struct page_directory_vec_s* page_dir_vec_init() {
    struct page_directory_vec_s* vec = calloc(sizeof(struct page_directory_vec_s), 1);
    vec->cap = 10;
    return vec;
}

void page_dir_vec_append(struct page_directory_vec_s* vec, page_directory* page_dir) {
    if (vec->size >= vec->cap) {
        vec->cap += 10;
        vec->vec = realloc(vec->vec, vec->cap);
    }
    vec->vec[vec->size] = page_dir;
    vec->size++;
}


uint8_t init_virtual_memory_manager(uint32_t kernel_address) {
    page_directory* dir = (page_directory*)allocate_blocks(3);

    if (!dir) {
        return 0; // Out of physical_memory_manager
    }


    memset(dir, 0, sizeof(page_directory));

    for (uint32_t i = 0; i < TABLE_PER_DIRECTORY; i++) {
        SET_ATTRIBUTE(&dir->entries[i], PAGE_DIR_ENTRY_READ_WRITE);
    }

    page_table* table = (page_table*)allocate_blocks(1);

    if (!table) {
        return 0;
    }

    // Allocate a 3GB page table
    page_table* table_higher_half = (page_table*)allocate_blocks(1);

    if (!table_higher_half) {
        return 0;
    }

    memset(table, 0, sizeof(page_table));
    memset(table_higher_half, 0, sizeof(page_table));

    // Identity map 1st 4MB of memory

    for (uint32_t i = 0, frame =0x0, virt = 0x0; i < PAGES_PER_TABLE; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        uint32_t page = 0;
        SET_ATTRIBUTE(&page, PAGE_TABLE_ENTRY_PRESENT);
        SET_ATTRIBUTE(&page, PAGE_TABLE_ENTRY_READ_WRITE);
        SET_FRAME(&page, frame);

        // Add page to 3GB page table
        table_higher_half->entries[PT_INDEX(virt)] = page;
    }

    // Map kernel to 3GB+

    for (uint32_t i = 0, frame = kernel_address, virt = 0xC0000000; i < PAGES_PER_TABLE; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        uint32_t page = 0;
        SET_ATTRIBUTE(&page, PAGE_TABLE_ENTRY_PRESENT);
        SET_ATTRIBUTE(&page, PAGE_TABLE_ENTRY_READ_WRITE);
        SET_FRAME(&page, frame);

        table->entries[PT_INDEX(virt)] = page;
    }

    uint32_t* entry = &dir->entries[PD_INDEX(0xC0000000)];
    SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_PRESENT);
    SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_READ_WRITE);
    SET_FRAME(entry, (uint32_t)table);  // 0xC0000000 point to default page table

    entry = &dir->entries[PD_INDEX(0x00000000)];
    SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_PRESENT);
    SET_ATTRIBUTE(entry, PAGE_DIR_ENTRY_READ_WRITE);
    SET_FRAME(entry, (uint32_t)table_higher_half); // 0x00000000 point to kernel page table

    if (!set_page_directory(dir)){
        return 0;
    }

    i686_ISR_Registerhandler(14, i686_Page_fault_handler);

    // Enable paging: Set paging bit (31) and protection enable bit (0) of CR0
    i686_enable_paging();

    page_dir_vec = page_dir_vec_init();
    return 1;
}

void* get_phys_addr(uint32_t virt_addr) {
    uint32_t* page = get_page(virt_addr);
    return (void*)(PAGE_PHYS_ADDR(page) + (virt_addr & 0xFFF));
}
