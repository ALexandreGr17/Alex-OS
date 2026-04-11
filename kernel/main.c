#include "log.h"

void kernel_main(void* multiboot_struct) {
    enable_cursor();
    clrscr();
    puts("test\n\t");
    logf("Hello %s%c %i %x", "world", '!', 10, 10);

    while(1);
}
