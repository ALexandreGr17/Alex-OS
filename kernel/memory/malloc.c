#include "memory_management/virtual/virtual_memory_manager.h"
#include <memory_management/memory_management.h>
#include <stdint.h>
#include <memory/memory.h>
#include <stdio.h>

#define STARTING_PAGE 0xF0000000

struct blk_header {
    struct blk_header* next;
    uint32_t           size;
    uint32_t           actual_size;
};

struct page_header {
    struct page_header* next;
    uint32_t            size;
    uint32_t           used_size;
    uint32_t           nb_block;
};

struct blk_header* free_list = NULL;
struct page_header* page_list = NULL;

void debug_free_list() {
    printf("########## Free list ##########\n");
    int i = 0;
    struct blk_header* tmp = free_list;
    while (tmp != NULL) {
        printf("    ### blk %d ###\n", i);
        printf("\tblk: %x\n", tmp);
        printf("\tnext: %x\n", tmp->next);
        printf("\tsize: %x\n", tmp->size);
        printf("\tactual_size: %x\n", tmp->actual_size);
        tmp = tmp->next;
        printf("\n");
        i++;
    }
    printf("\n\n");
}

void debug_page(struct page_header* page) {
    printf("###### Page ######\n");
    printf("page: %x\n", page);
    printf("next: %x\n", page->next);
    printf("size: %x\n", page->size);
    printf("used_size: %x\n", page->used_size);
    printf("nb_block: %x\n", page->nb_block);
    struct blk_header* blk = (void*)((uint32_t)page + sizeof(struct page_header));
    for (int i = 0; i < page->nb_block; i++) {
        printf("### blk %d ###\n", i);
        printf("blk: %x\n", blk);
        printf("next: %x\n", blk->next);
        printf("size: %x\n", blk->size);
        printf("actual_size: %x\n", blk->actual_size);
        blk = (void*)((uint32_t)blk + blk->actual_size + sizeof(struct blk_header));
    }
}

void debug_page_list() {
    struct page_header* tmp = page_list;
    while(tmp != NULL) {
        debug_page(tmp);
        tmp = tmp->next;
    }
}

uint32_t align(uint32_t n, uint32_t base) {
    if (n % base == 0) {
        return n;
    }
    return base - (n % base) + n;
}

int add_page(uint32_t size){
    uint32_t align_size = align(size, PAGE_SIZE);
    if (page_list == NULL) {
        page_list = allocate_new_page(STARTING_PAGE, align_size / PAGE_SIZE);
        if (page_list == NULL) {
            return 0;
        }
        page_list->size = align_size;
        page_list->used_size = sizeof(struct page_header);
        page_list->next = NULL;
        return 1;
    }
    struct page_header* tmp = page_list;
    while(tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = allocate_new_page((uint32_t)tmp + tmp->size, align_size / PAGE_SIZE);
    if (tmp->next == NULL) {
        return 0;
    }
    tmp = tmp->next;
    tmp->size = align_size;
    tmp->used_size = sizeof(struct page_header);
    tmp->next = NULL;
    return 1;
}

struct blk_header* find_free_blk(uint32_t size) {
    struct blk_header* free_blk = free_list;
    if (free_blk == NULL) {
        return NULL;
    }
    if (free_blk->actual_size > size) {
        free_list = free_blk->next;
        return free_blk;
    }
    while(free_blk->next != NULL && free_blk->next->actual_size < size) {
        free_blk = free_blk->next;
    }
    if (free_blk->next != NULL) {
        struct blk_header* tmp = free_blk->next;
        free_blk->next = tmp->next;
        return tmp;
    }
    return NULL;
}

void* malloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    uint32_t align_size = align(size, 0x10);
    uint32_t nb_page = align_size / PAGE_SIZE + (align_size % PAGE_SIZE == 0 ? 0 : 1);
    if (page_list == NULL) {
        if (!add_page(align_size)) {
            return NULL;
        }
    }

    struct blk_header* header = find_free_blk(align_size);
    if (header) {
        header->size = size;
        return (void*)((uint32_t)header + sizeof(struct blk_header));
    }


    struct page_header* tmp = page_list;
    while(tmp->next != NULL && tmp->size - tmp->used_size < align_size + sizeof(struct blk_header)) {
        tmp = tmp->next;
    }

    if (tmp->size - tmp->used_size < align_size + sizeof(struct blk_header)) {
        if(!add_page(align_size + sizeof(struct blk_header) + sizeof(struct page_header))){
            return NULL;
        }
        tmp = tmp->next;
    }

    header = (struct blk_header*)((uint32_t)tmp + tmp->used_size);
    header->size = size;
    header->actual_size = align_size;
    header->next = NULL;
    tmp->used_size += sizeof(struct blk_header) + header->actual_size;
    tmp->nb_block++;

    return (void*)((uint32_t)header + sizeof(struct blk_header));
}

/*
void *page_begin(void *ptr, uint32_t page_size)
{
    return (void*)((uint32_t)ptr & ~(page_size - 1));
}
*/
void free(void* ptr) {
    struct blk_header* header = (void*)((uint32_t)ptr - sizeof(struct blk_header)); // page_begin(ptr, header->actual_size);
    header->next = free_list;
    free_list = header;
}

void* calloc(uint32_t size, uint32_t nmenb) {
    uint8_t* ptr = malloc(size * nmenb);
    memset(ptr, 0, size * nmenb);
    return ptr;
}

void* realloc(void* ptr, uint32_t size) {
    if (size == 0) {
        free(ptr);
    }
    if (ptr == NULL) {
        return malloc(size);
    }
    struct blk_header* header = (void*)((uint32_t)ptr - sizeof(struct blk_header)); // page_begin(ptr, header->actual_size);
    if (header->actual_size < size) {
        header->size = size;
        return ptr;
    }
    void* new = malloc(size);
    memcpy(new, ptr, size);
    free(ptr);
    return new;
}
