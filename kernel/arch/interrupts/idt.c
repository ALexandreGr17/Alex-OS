#include "idt.h"
#include "pic.h"

#include <stdint.h>
#include <logs/log.h>

typedef struct idt_entry_s {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct idtr_s {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_descriptor_t;

__attribute__((aligned(0x10)))
static idt_entry_t idt[256];
static idt_descriptor_t idtr;

static uint8_t interrupts[256] = { 0 };
static interrupt_handler isr[256];

void exception_handler(interrupt_frame_t* frame) {
    if (interrupts[frame->id]) {
        isr[frame->id](frame);
        return;
    }
    if (frame->id < 32) {
        logf("KERNEL PANIC: Unhandeld interrupt %d\n", frame->id);
        for(;;);
    }
    logf("KERNEL WARNING: Unhandeld interrupt %d\n", frame->id);
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x8;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

void idt_set_interrupt(uint8_t intr, interrupt_handler handler) {
    interrupts[intr] = 1;
    isr[intr] = handler;
}

void idt_unset_interrupt(uint8_t intr) {
    interrupts[intr] = 0;
    isr[intr] = 0;
}

extern void* isr_stub_table[];

void idt_init() {
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt) - 1;

    for (uint16_t vector = 0; vector < 256; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr));

    pic_init();
    __asm__ volatile ("sti");
}
