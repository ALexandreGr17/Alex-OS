#ifndef VIRTUAL_MEMORY_MANAGER_H
#define VIRTUAL_MEMORY_MANAGER_H

#include <stdint.h>

#define NULL 0x0

#define PAGES_PER_TABLE 1024
#define TABLE_PER_DIRECTORY 1024
#define PAGE_SIZE 4069

#define PD_INDEX(virt_addr) ((virt_addr) >> 22)
#define PT_INDEX(virt_addr) ((virt_addr) >> 12) & 0x3FF
#define PAGE_PHYS_ADDR(dir_entry) ((*dir_entry) & ~0xFFF)
#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define CLR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
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

#endif
