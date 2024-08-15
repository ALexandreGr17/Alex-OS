#include "memory.h"
#include <stdint.h>

void* memcpy(void* dst, const void* src, uint16_t n){
	for(int i = 0; i < n; i++){
		((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
	}
	return dst;
}

void* memset(void* ptr, int value, uint16_t n){
	for(int i = 0; i < n; i++){
		((uint8_t*)ptr)[i] = (uint8_t)value;
	}
	return ptr;
}

int memcmp(const void *ptr1, const void *prt2, uint16_t n){
	for(int i = 0; i < n; i++){
		if(((uint8_t*)ptr1)[i] != ((uint8_t*)prt2)[i]){
			return 1;
		}
	}
	return 0;
}

void* segoff_to_linear(void* addr){
	uint32_t off = (uint32_t)(addr) & 0xFFFF;
	uint32_t seg = (uint32_t)(addr) >> 16;
	return (void*)(seg * 16 + off);
}
