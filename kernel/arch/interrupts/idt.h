#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct interrupt_frame_s {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t id;
    uint64_t error;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} __attribute__((packed)) interrupt_frame_t;
typedef void (*interrupt_handler)(interrupt_frame_t*);

void idt_init();
void idt_unset_interrupt(uint8_t intr);
void idt_set_interrupt(uint8_t intr, interrupt_handler handler);

#endif
