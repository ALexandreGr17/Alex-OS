#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include <stdint.h>


typedef struct bootstrap_info_s {
    uint64_t* pmm_map;
    uint64_t pmm_map_size;
    void* multiboot_struct;
} bootstrap_info_t;

typedef void (*entry_point_t)(bootstrap_info_t*);

#endif // !BOOTSTRAP_H
