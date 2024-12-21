#include <stdint.h>

extern uint32_t __end;

uint32_t last_addr;
uint32_t** stack;
uint32_t head;

#define GET_BIT(val, bit) (val & (1 << bit)) >> bit
#define SET_BIT(val, bit) val | (1 << bit) 
#define UNSET_BIT(val, bit) val & ~(1 << bit) 

void Frame_allocator_init() {
    *stack = (uint32_t*)__end;
}

void stack_push(uint32_t val) {
    ++head;
    (*stack)[head] = val;
}

uint32_t stack_pop() {
    return (*stack)[--head];
}

uint32_t align2(uint32_t addr) {
    if (addr % 0x1000 == 0) {
        return addr;
    }
    return (0x1000 - (addr % 0x1000)) + addr;
}

void add_new_frame() {
    uint32_t next = align2(last_addr);
    SET_BIT(next, 0);
    stack_push(next);
}
