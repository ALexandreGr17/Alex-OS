#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#include "fat.h"
#include "mbr.h"

typedef struct {
	uint8_t		signature[4];
	uint8_t		bitness;
	uint8_t		endianness;
	uint8_t		version;
	uint8_t		abi;
	uint8_t		unused[8];
	uint16_t	type;
	uint16_t	instruction_set;
	uint32_t    elf_version;
	uint32_t    program_entry;
	uint32_t	header_table;
	uint32_t	section_table;
	uint32_t	flag;
	uint16_t	header_size;
	uint16_t	header_entry_size;
	uint16_t	header_entry_count;
	uint16_t	section_entry_size;
	uint16_t	section_entry_count;
	uint16_t	section_names_index;
} __attribute__((packed)) elf_hdr_t;

typedef struct {
	uint32_t	type;
	uint32_t	p_offset;
	uint32_t	p_vaddr;
	uint32_t	p_addr;
	uint32_t	p_filez;
	uint32_t	p_memsz;
	uint32_t	flags;
	uint32_t	alignment;
} __attribute__((packed)) pgr_hdr_entry_t;

enum ELF_BITNESS {
	ELF_BITNESS_32 = 1,
	ELF_BITNESS_64 = 2,
};

enum ELF_ENDIANESS { 
	ELF_LITTLE_ENDIAN	= 1,
	ELF_BIG_ENDIAN		= 2,
};

enum ELF_INSTRUCTION_SET {
	ELF_INSTRUCTION_SET_NO_SPEC		= 0,
	ELF_INSTRUCTION_SET_SPARC		= 0x02,
	ELF_INSTRUCTION_SET_X86			= 0x3,
	ELF_INSTRUCTION_SET_MIPS		= 0x8,
	ELF_INSTRUCTION_SET_POWER_PC	= 0x14,
	ELF_INSTRUCTION_SET_ARM			= 0x28,
	ELF_INSTRUCTION_SET_SUPERH		= 0x2A,
	ELF_INSTRUCTION_SET_IA_64		= 0x32,
	ELF_INSTRUCTION_SET_X86_64		= 0x3E,
	ELF_INSTRUCTION_SET_AARCH64		= 0xB7,
	ELF_INSTRUCTION_SET_RISC_V		= 0xF3,
};

enum ELF_TYPE {
	ELF_TYPE_RELOCATABLE	= 1,
	ELF_TYPE_EXECUTABLE		= 2,
	ELF_TYPE_SHARED			= 3,
	ELF_TYPE_CORE			= 4,
};

int ELF_open(partition_t *part, FAT_file *fd);

#endif
