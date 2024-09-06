#include "memory.h"
#include <stdint.h>


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
