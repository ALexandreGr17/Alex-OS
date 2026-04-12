#include "logs/log.h"
#include <stdint.h>
#include <memory/memory.h>

void kernel_main(void* multiboot_struct) {
    enable_cursor();
    clrscr();
    puts("test\n\t");
    logf("Hello %s%c %i %x\n", "world", '!', 10, 10);
    init_pmm(multiboot_struct);

    while(1);
}
