#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#include <stdint.h>

void* memcpy(void* dest, void* src, uint64_t size);
void* memset(void* dest, uint8_t val, uint64_t size);
uint8_t memcmp(void* dest, void* src, uint64_t size);

#endif
