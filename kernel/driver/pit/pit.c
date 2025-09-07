
#include "arch/i686/pic.h"
#include <stdint.h>
#include <arch/i686/io.h>
#include <arch/i686/isr.h>
#include <stdio.h>

#define PIT_CHAN_0_DATA 0x40
#define PIT_CHAN_1_DATA 0x41
#define PIT_CHAN_2_DATA 0x42
#define PIT_CMD 0x43

#define MS_FREQ 1000
#define PIT_FREQ 1193182 

static uint32_t pit_ticks = 0;

void PIT_handler(Register* regs) {
    pit_ticks++;
    i686_PIC_SendEOI(0);
}

void PIT_init() {
    uint16_t divisor = PIT_FREQ / MS_FREQ;

    i686_outb(PIT_CMD, 0x36);
    i686_outb(PIT_CHAN_0_DATA, divisor & 0xFF);
    i686_outb(PIT_CHAN_0_DATA, (divisor >> 8) & 0xFF);

    printf("PIT -------------------------------------------------\n");
    i686_ISR_Registerhandler(0x20, PIT_handler);
}

void sleep_ms(uint32_t ms) {
    uint32_t target = pit_ticks + ms;
    while(pit_ticks != target) __asm__("hlt");
}
