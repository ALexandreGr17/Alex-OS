#include "heap.h"
#include <boot/bootparams.h>
#include <stdint.h>
#include <stdio.h>
#include "x86.h"

memory_info_t* mem_info = NULL;
uint8_t current_block = 0;
heap_t* heap = NULL;

heap_t* get_heap(){
	return heap;
}

void* get_next_free_block(memory_region_t* mem_regs, uint64_t* size){
	do {
		if(mem_regs[current_block].Type == MEMORY_TYPE_USABLE){
			*size = mem_regs[current_block].Length;
			return (void*)mem_regs[current_block].Begin;
		}
		current_block--;
	}while(current_block >= 0);
	*size = 0;
	return NULL;
}

void init_memory_management(memory_info_t* mem_info){
	current_block = mem_info->region_count-1;
	mem_info = mem_info;
	uint64_t size = 0;
	void* first_block = get_next_free_block(mem_info->regions, &size);
	heap = first_block;
	heap->block_strart = first_block;
	heap->block_prev = NULL;
	heap->block_size = size;
	heap->used_size = sizeof(heap_t);
	heap->last_header = NULL;
	heap->first_header = NULL;
}


void debug_heap(){
	printf("\n----------------\nHEAP:\n");
	printf("start: 0x%lx\n", heap->block_strart);
	printf("size: 0x%lx\n", heap->block_size);
	printf("used_size: 0x%lx\n", heap->used_size);
	printf("first header: 0x%lx\n",heap->first_header);
	printf("last header: 0x%lx\n",heap->last_header);
	printf("\n----------------\n");
}


/*
void increment_heap(){
	uint64_t size = 0;
	void* next_block = get_next_free_block(mem_info->regions, &size);
	heap->block_prev = heap->block_strart;
	heap->block_strart = next_block;
	heap->block_size = size;
	heap->used_size = 0;
}

void decrement_heap(){
	heap->block_strart = heap->block_prev;
	heap->block_size = 
}*/
