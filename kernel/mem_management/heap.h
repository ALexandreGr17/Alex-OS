#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

typedef struct {
	void* next;
	void* prev;
	uint64_t size;
	uint64_t used_size;
	uint8_t free;
} memory_header_t;

typedef struct {
	void* block_prev;
	void* block_strart;
	uint64_t block_size;
	uint64_t used_size;
	memory_header_t* first_header;
	memory_header_t* last_header;
} heap_t;

#define NULL 0

heap_t* get_heap();

#endif
