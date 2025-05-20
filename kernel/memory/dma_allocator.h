#ifndef DMA_ALLOCATOR_H
#define DMA_ALLOCATOR_H

#include <stdint.h>

void* dma_alloc(uint32_t size, uint32_t alignment);
void dma_free(void* ptr);

#endif
