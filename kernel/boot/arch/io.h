#ifndef BOOTSTRAP_IO_H
#define BOOTSTRAP_IO_H

#include <stdint.h>
uint8_t bootstrap_io_in8(uint16_t port);
uint16_t bootstrap_io_in16(uint16_t port);
uint32_t bootstrap_io_in32(uint16_t port);

void bootstrap_io_out8(uint16_t port, uint8_t data);
void bootstrap_io_out16(uint16_t port, uint16_t data);
void bootstrap_io_out32(uint16_t port, uint32_t data);

#endif
