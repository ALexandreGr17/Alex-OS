#include "memory.h"
#include "memory_management/memory_management.h"
#include "memory_management/virtual/virtual_memory_manager.h"
#include <stdint.h>
#include <stdio.h>


void memcpy(void* dst, const void* src, uint32_t size){
	for(int i = 0; i < size; i++){
		((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
	}
}

void memset(void *ptr, int value, uint32_t n){
	for(int i = 0; i < n; i++){
		((uint8_t*)ptr)[i] = (uint8_t)value;
	}

}

uint8_t memcmp(const void *ptr1, const void *prt2, uint32_t n){
	for(int i = 0; i < n; i++){
		if(((uint8_t*)ptr1)[i] != ((uint8_t*)prt2)[i]){
			return 0;
		}
	}
	return 1;
}

void* mmap(void* address, uint32_t len) {
    if (!address) {
        address = find_free_page(NB_PAGE(len));
    }

    if (!address) {
        return NULL;
    }
    return allocate_new_page((uint32_t)address, NB_PAGE(len));
}

void munmap(void* address, uint32_t len) {
    deallocate_page((uint32_t)address, NB_PAGE(len));
}

