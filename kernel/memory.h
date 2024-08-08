#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
void* memcpy(void* dst, const void* src, uint16_t n);
void* memset(void* ptr, int value, uint16_t n);
int memcmp(const void* ptr1, const void* prt2, uint16_t n);

#endif
