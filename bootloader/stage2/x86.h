#ifndef X86_H
#define X86_H

#include <stdint.h>

typedef struct {
	uint64_t Base;
	uint64_t Length;
	uint32_t Type;
	uint32_t ACPI;
} E820MemoryBlock;

enum E820MemoryBlockType {
	E820_USABLE = 1,
	E820_RESERVED = 2,
	E820_ACPI_RECLAIMABLE = 3,
	E820_ACPI_NVS = 4,
	E820_BAD_MEMORY = 5,
};

void __attribute__((cdecl)) x86_outb(uint16_t port, uint8_t value); // send value to port 
uint8_t __attribute__((cdecl)) x86_inb(uint16_t port); // read value from port

uint8_t __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive, uint8_t* drive_type, uint16_t* nb_cylinders, uint16_t* nb_sectors, uint16_t* nb_heads);
uint8_t __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);
uint8_t __attribute__((cdecl)) x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint16_t sector_count, void* output_buffer);

int __attribute__((cdecl)) x86_E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationID);

#endif
