#ifndef MEM_MANAGEMENT
#define MEM_MANAGEMENT

#include <stdint.h>
#include <boot/bootparams.h>

void init_memory_management(memory_info_t* mem_info);
void* malloc(uint64_t size);
void free(void* ptr);

void debug_heap();
void debug_header(void* ptr);
#endif
