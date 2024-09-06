#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#define NULL 0

void memcpy(void* dst, const void* src, uint32_t n);
void memset(void *ptr, int value, uint32_t n);
uint8_t memcmp(const void *ptr1, const void *prt2, uint32_t n);
#endif
