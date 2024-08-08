#ifndef X86_H
#define X86_H

#include <stdint.h>

void __attribute__((cdecl)) x86_outb(uint16_t port, uint8_t value); // send value to port 
uint8_t __attribute__((cdecl)) x86_inb(uint16_t port); // read value from port

uint8_t __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive, uint8_t* drive_type, uint16_t* nb_cylinders, uint16_t* nb_sectors, uint16_t* nb_heads);
uint8_t __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);
uint8_t __attribute__((cdecl)) x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint16_t sector_count, void* output_buffer);

#endif
