#include "arch/interrupts/idt.h"
#include "arch/interrupts/pic.h"
#include "arch/io.h"
#include <stdint.h>
#include <logs/log.h>

#define PIT_CHAN_0_DATA 0x40
#define PIT_CHAN_1_DATA 0x41
#define PIT_CHAN_2_DATA 0x42
#define PIT_CMD 0x43

#define MS_FREQ 1000
#define PIT_FREQ 1193182

static uint64_t pit_ticks = 0;

void pit_handler(interrupt_frame_t* frame) {
    (void)frame;
    pit_ticks++;
    pic_send_eoi(0);
}

void pit_init() {
    uint16_t divisor = PIT_FREQ / MS_FREQ;

    io_out8(PIT_CMD, 0x36);
    io_out8(PIT_CHAN_0_DATA, divisor & 0xFF);
    io_out8(PIT_CHAN_0_DATA, (divisor >> 8) & 0xFF);

    pic_enable_irq(0, pit_handler);
}

void sleep_ms(uint64_t ms) {
    uint64_t target = pit_ticks + ms;
    while (pit_ticks != target) __asm__("hlt");
}
