#include <arch/i686/io.h>
#include <stdint.h>
#define PORT_CTRL 0x70
#define PORT_DATA 0x71

#define NMI_DISALED_BIT 7

uint8_t CMOS_read_reg(uint8_t reg) {
    i686_outb(PORT_CTRL, (1 << NMI_DISALED_BIT) | reg);
    for(int i = 0; i < 10; i++) {
        i686_iowait();
    }
    return i686_inb(PORT_DATA);
}
