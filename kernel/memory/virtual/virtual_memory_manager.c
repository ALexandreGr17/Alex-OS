#include <stdint.h>
#include <memory/physical/physical_memory_manager.h>
#include <memory/mem_utils.h>

static uint64_t* PML4;
static uint64_t* PDPT;
static uint64_t* PD;
static uint64_t* PT;

extern void* get_cr3();

void init_vmm() {
    PML4 = get_cr3();
    PDPT = (uint64_t*)*PML4;
    PD = (uint64_t*)*PDPT;
}
