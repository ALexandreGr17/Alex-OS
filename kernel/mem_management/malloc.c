#include "heap.h"
#include <stdint.h>
#include <stdio.h>
#include "x86.h"

void* get_data_block(memory_header_t* header){
	return header +1; //+ sizeof(memory_header_t);
}

memory_header_t* get_header(void* block){
	return (memory_header_t*)block - 1;
}

uint64_t align(uint64_t size){
	return size % 8 == 0 ? size : (size + (8 - size % 8));
}

memory_header_t* expand(uint64_t size){
	heap_t* heap = get_heap();
	void* next_addr = heap->block_strart + heap->used_size;
	
	if(next_addr + size  > heap->block_strart + heap->block_size){
		printf("expend fail 1\n");
		printf("next_addr: 0x%lx, size: 0x%x, block_start: 0x%lx, blocksize: 0x%x\n", 
				next_addr, size, heap->block_strart, heap->block_size);
		debug_heap();
		return NULL;
	}
	heap->used_size += size + sizeof(memory_header_t);
	memory_header_t* mem_header = next_addr;
	mem_header->prev = heap->last_header;
	heap->last_header->next = mem_header;
	heap->last_header = mem_header;
	if(heap->first_header == NULL){
		heap->first_header = mem_header;
	}
	mem_header->size = size;
	mem_header->next = NULL;
	return mem_header;
}

void* malloc(uint64_t size){
	heap_t* heap = get_heap();
	uint64_t align_size = align(size);
	memory_header_t* header = heap->first_header;
	while(header != NULL){
		if(header->free && header->size <= align_size){
			header->used_size = size;
			return get_data_block(header);
		}
		header = header->next;
	}
	header = expand(align_size);
	if(header == NULL){
		return NULL;
	}
	header->used_size = size;
	header->free = 0;
	return get_data_block(header);
}


void* calloc(uint64_t size, uint8_t val){
	uint8_t* buf = malloc(size);
	for(int i = 0; i < size; i++){
		buf[i] = val;
	}
	return buf;
}

void debug_header(void* ptr){
	memory_header_t* header = get_header(ptr);
	if(header == NULL){
		return;
	}
	printf("0x%x\n", sizeof(memory_header_t));
	printf("\n-------------------\nHEADER:\n");
	printf("Start: 0x%lx\n", header);
	printf("Data Start: 0x%lx\n", header + 0x1);
	printf("size: 0x%lx\n", header->size);
	printf("used size: 0x%lx\n", header->used_size);
	printf("next: 0x%lx\n", header->next);
	printf("prev: 0x%lx\n", header->prev);
	printf("free: 0x%x\n", header->free);
}

void free(void* ptr){
	memory_header_t* mem_header = get_header(ptr);
	mem_header->free = 1;
}
