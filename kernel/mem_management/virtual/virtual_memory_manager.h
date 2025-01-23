#ifndef VIRTUAL_MEMORY_MANAGER_H
#define VIRTUAL_MEMORY_MANAGER_H

#include <stdint.h>
#include <string.h>
#include "mem_management/physique/physical_memory_manager.h"

#define PAGES_PER_TABLE 1024
#define TABLE_PER_DIRECTORY 1024
#define PAGE_SIZE 4069

#define PD_INDEX(virt_addr) ((virt_addr) >> 22)
#define PT_INDEX(virt_addr) ((virt_addr) >> 12) & 0x3FF
#define PAGE_PHYS_ADDR(dir_entry) ((*dir_entry) & ~0xFFF)
#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define UNSET_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TST_ATTRIBUTE(entry, attr) (*entry & ~attr)
#define SET_FRAME(entry, addr) (*entry = (*entry & ~0x7FFFF000) | addr)

enum PAGE_TABLE_FLAGS {
    PAGE_TABLE_ENTRY_PRESENT                = 0x01,
    PAGE_TABLE_ENTRY_READ_WRITE             = 0x02,
    PAGE_TABLE_ENTRY_USER                   = 0x04,
    PAGE_TABLE_ENTRY_WRITE_THROUGH          = 0x08,
    PAGE_TABLE_ENTRY_CACHE_DISABLE          = 0x10,
    PAGE_TABLE_ENTRY_CACHE_ACCESSED         = 0x20,
    PAGE_TABLE_ENTRY_CACHE_DIRTY            = 0x40,
    PAGE_TABLE_ENTRY_PAGE_ATTRIBUTE_TABLE   = 0x80,
    PAGE_TABLE_ENTRY_GLOBAL                 = 0x100,
    PAGE_TABLE_ENTRY_FRAME                  = 0x7FFFF000,
};

enum PAGE_DIR_FLAGS {
    PAGE_DIR_ENTRY_PRESENT                = 0x01,
    PAGE_DIR_ENTRY_READ_WRITE             = 0x02,
    PAGE_DIR_ENTRY_USER                   = 0x04,
    PAGE_DIR_ENTRY_WRITE_THROUGH          = 0x08,
    PAGE_DIR_ENTRY_CACHE_DISABLE          = 0x10,
    PAGE_DIR_ENTRY_CACHE_ACCESSED         = 0x20,
    PAGE_DIR_ENTRY_CACHE_DIRTY            = 0x40,
    PAGE_DIR_ENTRY_PAGE_SIZE              = 0x80,
    PAGE_DIR_ENTRY_GLOBAL                 = 0x100,
    PAGE_DIR_ENTRY_PAGE_ATTRIBUTE_TABLE   = 0x200,
    PAGE_DIR_ENTRY_FRAME                  = 0x7FFFF000,
};

typedef struct {
    uint32_t entries[PAGES_PER_TABLE];
} page_table;

typedef struct {
    uint32_t entries[TABLE_PER_DIRECTORY];
} page_directory;

page_directory* current_page_dir = NULL;
uint32_t current_pd_address = NULL;

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

    __asm__ __volatile__ ("movl %%EAX, %%CR3" : : "a"(current_page_dir) );

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

void unmap_page(void* virtual_address) {
    uint32_t* page = get_page((uint32_t)virtual_address);
    SET_FRAME(page, 0);
    UNSET_ATTRIBUTE(page, PAGE_TABLE_ENTRY_PRESENT);
}

uint8_t init_virtual_memory_manager(void) {
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
}

#endif 
