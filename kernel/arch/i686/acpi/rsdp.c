#include "./rsdp.h"
#include "memory/memory.h"
#include <stdint.h>

uint8_t check_RDSP(void* addr){
	char* sig = "RSD PTR ";

	RSDP_t* rdsp = (RSDP_t*)addr;
	uint8_t check = 0;
	uint8_t* addr_8b = addr;

	if(memcmp(rdsp->signature, sig, 8)){		// check the signature
		// calculate the checksum
		for(int i = 0; i < sizeof(RSDP_t); i++){
			check += *addr_8b;
			addr_8b++;
		}

		if(check == 0){
			return 1;
		}
	}
	return 0;
}

void find_RSDP(RSDP_t** rsdp){
	uint32_t addr = 0x000E0000;

	for(; addr <= 0x000FFFFF; addr += 0x10/sizeof(addr)){
		if(check_RDSP((void*)addr)){
			*rsdp = (RSDP_t*)addr;
			return;
		}
	}

	int ebda = *((uint16_t*)0x40E);
	ebda = ebda * 0x10 & 0x000FFFFF;

	for(addr = ebda; addr < ebda + 1024; addr += 0x10/sizeof(addr)){
		if(check_RDSP((void*)addr)){
			*rsdp = (RSDP_t*)addr;
			return;
		}
	}
}
