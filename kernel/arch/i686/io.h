#ifndef IO_H
#define IO_H

#include <stdint.h>
// 8-bit
void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);
// 16-bit
void __attribute__((cdecl)) i686_outw(uint16_t port, uint16_t value);
uint16_t __attribute__((cdecl)) i686_inw(uint16_t port);
// 32-bit
void __attribute__((cdecl)) i686_outl(uint16_t port, uint32_t value);
uint32_t __attribute__((cdecl)) i686_inl(uint16_t port);

void __attribute__((cdecl)) i686_DisableInterrupts();
void __attribute__((cdecl)) i686_EnableInterrupts();

void __attribute__((cdecl)) i686_iowait();
void __attribute__((cdecl)) i686_panic();
#endif
