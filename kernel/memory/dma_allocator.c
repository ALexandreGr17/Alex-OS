#include <memory_management/physique/physical_memory_manager.h>
#include <memory_management/virtual/virtual_memory_manager.h>
#include <stdint.h>
#include <stdio.h>


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define NB_PAGE(size) (size / PAGE_SIZE + (size % PAGE_SIZE == 0 ? 0 : 1))

struct blk_header {
    struct blk_header* prev;
    uint32_t           size;
    uint32_t           actual_size;
    uint8_t            free;
    void*              blk;
};

struct page_header {
    struct page_header* next;
    uint32_t            size;
    uint32_t            used_size;
    uint32_t            phys_addr;
    void*               page;
    struct blk_header* last_blk;
};

struct page_header* first_page = NULL;

uint32_t dma_align(uint32_t n, uint32_t base) {
    if (n % base == 0) {
        return n;
    }
    return base - (n % base) + n;
}

void* dma_allocate_new_page(uint32_t nb_page) {
    void* phys_addr = allocate_blocks(nb_page);
    if (!phys_addr) {
        return NULL;
    }

    void* virt_addr = find_free_page(nb_page);

    for (uint32_t i = 0; i < nb_page; i++) {
        map_page(phys_addr + i * PAGE_SIZE, virt_addr + i * PAGE_SIZE);
    }
    return virt_addr;
}

void dma_deallocate_page(void* page, uint32_t phys_addr, uint32_t nb_page) {
    uint32_t base_addr = (uint32_t)page;

    for (uint32_t i = 0; i < nb_page; i++) {
        unmap_page((void*)base_addr);
        base_addr += PAGE_SIZE;
    }
    free_blocks((void*)phys_addr, nb_page);
}

void* dma_alloc(uint32_t size, uint32_t alignment) {
    uint32_t align_size = dma_align(size, alignment);
    struct page_header* page = NULL;
    uint32_t position = -1;
    if (first_page == NULL) {
        void* new_page = dma_allocate_new_page(NB_PAGE(size));
        first_page = calloc(sizeof(struct page_header), 1);
        first_page->page = new_page;
        first_page->phys_addr = (uint32_t)get_phys_addr((uint32_t)new_page);
        first_page->size = NB_PAGE(size) * PAGE_SIZE;
        position = 0;
    }
    page = first_page;

    struct blk_header* blk = NULL;
    while (page->next != NULL) {
        if (page->last_blk == NULL) {
            position = 0;
            break;
        }
        blk = page->last_blk;
        while (blk) {
            if (blk->free && (uint32_t)blk->blk % alignment == 0 && blk->actual_size >= align_size) {
                break;
            }
            blk = blk->prev;
        }
        if (!blk) {
            break;
        }

        position = (uint32_t)page->last_blk->blk - (uint32_t)page->page;
        if (page->last_blk->actual_size % alignment) {
            size += dma_align(page->last_blk->actual_size, alignment);
        }
        if (position + align_size < page->size) {
            page->last_blk->actual_size = dma_align(page->last_blk->actual_size, alignment);
            break;
        }
        page = page->next;
    }
    if (blk) {
        blk->free = 0;
        return blk->blk;
    }
    if (position >= page->size) {
        void* new_page = dma_allocate_new_page(NB_PAGE(size));
        page->next = calloc(sizeof(struct page_header), 1);
        page = page->next;
        page->phys_addr = (uint32_t)get_phys_addr((uint32_t)new_page);
        page->size = NB_PAGE(size) * PAGE_SIZE;
        page->page = new_page;
        position = 0;
    }

    struct blk_header* new_blk = calloc(sizeof(struct blk_header), 1);
    new_blk->size = size;
    new_blk->actual_size = align_size;
    new_blk->prev = page->last_blk;
    page->last_blk = new_blk;
    new_blk->blk = (void*)((uint32_t)page->page + position);
    return new_blk->blk;
}

void dma_free(void* ptr) {
    struct page_header* page = first_page;
    struct blk_header* blk = NULL;
    while (page) {
        blk = page->last_blk;
        while (blk) {
            if (blk->blk == ptr) {
                break;
            }
            blk = blk->prev;
        }
        page = page->next;
    }
    if (!blk) {
        return;
    }
    blk->free = 1;
    if (blk == page->last_blk) {
        page->last_blk = blk->prev;
        free(blk);
    }
    if (!page->last_blk) {
        dma_deallocate_page(page->page, page->phys_addr, NB_PAGE(page->size));
    }
}
