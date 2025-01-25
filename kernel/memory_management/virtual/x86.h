#ifndef X86_VIRTUAL_MEM_H
#define X86_VIRTUAL_MEM_H

#include <stdint.h>
#include "virtual_memory_manager.h"

void i686_enable_paging();
void __attribute__((cdecl)) i686_load_page_dir(page_directory* pd);
uint32_t i686_get_cr2();


#endif
