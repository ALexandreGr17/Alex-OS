#include <memory/mem_utils.h>
#include <stdint.h>

void* memcpy(void* dest, void* src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    }

    return dest;
}

void* memset(void* dest, uint8_t val, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((uint8_t*)dest)[i] = val;
    }

    return dest;
}

uint8_t memcmp(void* dest, void* src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        if (((uint8_t*)dest)[i] != ((uint8_t*)src)[i]) {
            return 0;
        }
    }

    return 1;
}

