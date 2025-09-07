#include "./acpi.h"
#include "./rsdp.h"
#include "arch/i686/io.h"
#include "memory/memory.h"
#include "stdio.h"
#include "string/string.h"
#include <stddef.h>
#include <stdint.h>


RSDP_t* rsdp = NULL;

char acpi_check_header(SDT_hdr_t* hdr, char sig[4])
{
	return memcmp(hdr->signature, sig, 4);
}

SDT_hdr_t* find_descriptor_table(char signature[4]){
	size_t nb_entry = 0;
	if(rsdp->revision == 0){
		SDT_hdr_t* rsdt = (SDT_hdr_t*)rsdp->RSDT_address;
		nb_entry = rsdt->length - sizeof(SDT_hdr_t) / 4;
	}
	else{
		SDT_hdr_t* xsdt = (SDT_hdr_t*)rsdp->XSDT_address;
		nb_entry = xsdt->length - sizeof(SDT_hdr_t) / 8;
	}

	for(int i = 0; i < nb_entry; i++){
		if (rsdp->revision == 0){	
			SDT_hdr_t* rsdt = (SDT_hdr_t*)rsdp->RSDT_address;
			uint32_t* entrys = (uint32_t*)(rsdt + sizeof(SDT_hdr_t));
			if (acpi_check_header((SDT_hdr_t*)entrys[i], signature))
			{
				return (SDT_hdr_t*)entrys[i];
			}
		}
		else{
			SDT_hdr_t* xsdt = (SDT_hdr_t*)rsdp->RSDT_address;
			uint64_t* entrys = (uint64_t*)(xsdt + sizeof(SDT_hdr_t));
			if (acpi_check_header((SDT_hdr_t*)entrys[i], signature))
			{
				return (SDT_hdr_t*)entrys[i];
			}
		}
	}
	return NULL;
}

void acpi_init(){
	find_RSDP(&rsdp);

	printf("ACPI version: %s\n", rsdp->revision == 0 ? "1.0" : "2.0 - 6.4");

	if(rsdp == NULL || 
			(!acpi_check_header((SDT_hdr_t*)rsdp->RSDT_address, "RSDT") 
			 && !acpi_check_header((SDT_hdr_t*)rsdp->XSDT_address, "XSDT"))){
		printf("no ACPI\n");
		printf("RSDP :0x%x\n", rsdp);
		return;
	}
}
