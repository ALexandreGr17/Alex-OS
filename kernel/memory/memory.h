#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#define NULL 0

void memcpy(void* dst, const void* src, uint32_t n);
void memset(void *ptr, int value, uint32_t n);
uint8_t memcmp(const void *ptr1, const void *prt2, uint32_t n);
void* malloc(uint32_t size);
void* calloc(uint32_t size, uint32_t nmenb);
void* realloc(void* ptr, uint32_t size);
void free(void* ptr);

#endif
