#include "logs/log.h"
#include <stdint.h>
#include <memory/memory.h>
#include <memory/physical/physical_memory_manager.h>

void kernel_loop() {
    clrscr();
    puts("test\n\t");
    logf("Hello %s%c %i %x\n", "world", '!', 10, 10);
    
    uint8_t* buf = (uint8_t*)0x40;
    logf("%x\n", vmm_map_page);
    logf("%x\n", logf);
    vmm_map_page(buf, pmm_find_free_block(1), 1);
    // buf[0] = 'a';
    // buf[1] = 0;
    // logf("%s\n", buf);
}


void kernel_main(void* multiboot_struct) {
    enable_cursor();
    clrscr();
    puts("test\n\t");
    logf("Hello %s%c %i %x\n", "world", '!', 10, 10);
    init_pmm(multiboot_struct);
    init_vmm_lower(kernel_loop);

    while(1);
}
