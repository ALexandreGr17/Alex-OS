#include <boot/memory/physical.h>
#include <boot/memory/virtual.h>
#include <boot/log/logf.h>
#include <boot/bootstrap.h>
#include <stdint.h>

extern void kernel_main(bootstrap_info_t*);

static char __attribute__((section(".bootstrap_data"))) text[] = "Alexos starting...\n";
void __attribute__((section(".bootstrap"))) bootstrap_main(void* multiboot_struct) {
    enable_cursor();
    bootstrap_clrscr();
    bootstrap_logf(text);
    uint64_t pmm_map_size = 0;
    uint64_t* pmm_map = init_pmm(multiboot_struct, &pmm_map_size);

    bootstrap_info_t info = {
        .multiboot_struct = multiboot_struct,
        .pmm_map = pmm_map,
        .pmm_map_size = pmm_map_size
    };

    init_vmm(kernel_main, &info);
}
