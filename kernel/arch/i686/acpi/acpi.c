#include "./rsdp.h"
#include "arch/i686/io.h"
#include "memory/memory.h"
#include "stdio.h"
#include <stdint.h>

typedef struct {
	char		signature[4];
	uint32_t	length;
	uint8_t		_reserved[40-8];
	uint32_t*	DSDT;
	uint8_t		_reserved2[48 - 44];
	uint32_t*	SMI_CMD;
	uint8_t		ACPI_ENABLE;
	uint8_t		ACPI_DISABLE;
	uint8_t		_reserved3[64-54];
	uint32_t*	PM1a_CNT_BLK;
	uint32_t*	PM1b_CNT_BLK;
	uint8_t		_reserved4[89-72];
	uint8_t		PM1_CNT_LEN;
} __attribute__((packed)) FACP_t;

uint32_t* SMI_CMD;
uint8_t   ACPI_ENABLE;
uint8_t   ACPI_DISABLE;
uint32_t* PM1a_CNT_BLK;
uint32_t* PM1b_CNT_BLK;
uint16_t  SLP_TYPa;
uint16_t  SLP_TYPb;
uint16_t  SLP_EN;
uint16_t  SCI_EN;
uint8_t   PM1_CNT_LEN;


uint8_t acpi_check_header(uint32_t* ptr, char* sig){
	if(memcmp(ptr, sig, 4)){ // check the signature
		// calculate the checksum
		uint8_t* ptr_8b = (uint8_t*)ptr;
		char check = 0;
		for(int i = 0; i < *(ptr+ 1); i++){
			check += *ptr_8b;
			ptr_8b++;
		}

		if(check == 0){
			return 1;
		}
	}
	return 0;
}

void parsing_s5_obj(char* s5_addr, FACP_t* facp){
	s5_addr+=5;
	s5_addr += ((*s5_addr & 0xC0) >> 6) + 2;

	if(*s5_addr == 0x0A){
		s5_addr++;
	}
						
	SLP_TYPa = *(s5_addr) << 10;
	s5_addr++;

	if(*s5_addr == 0x0A){
		s5_addr++;
	}
						
	SLP_TYPb = *(s5_addr) << 10;
	s5_addr++;
						
	SMI_CMD = facp->SMI_CMD;

	ACPI_ENABLE = facp->ACPI_ENABLE;
	ACPI_DISABLE = facp->ACPI_DISABLE;

	PM1a_CNT_BLK = facp->PM1a_CNT_BLK;
	PM1b_CNT_BLK = facp->PM1b_CNT_BLK;

	PM1_CNT_LEN = facp->PM1_CNT_LEN;

	SLP_EN = 1 << 13;
	SCI_EN = 1;


}

void acpi_init(){
	RSDP_t* rdsp = 0;
	find_RSDP(&rdsp);

	if(rdsp == NULL || !acpi_check_header((uint32_t*)rdsp->RSDT_address, "RSDT")){
		printf("no RSDT\n");
		return;
	}

	uint32_t* rsdt = (uint32_t*)rdsp->RSDT_address;

	// calculating the number of entries
	int entrys = *(rsdt + 1);
	entrys = (entrys-36)/4;
	rsdt += 36/4; // skipping header
	
	while(entrys-- > 0){
		if(acpi_check_header(*rsdt, "FACP")){
			entrys -= 2;
			FACP_t* facp = (FACP_t*)*rsdt;
			if(acpi_check_header(facp->DSDT, "DSDT")){
				char* S5_addr = (char*)facp->DSDT + 36; // skipping header of DSDT;
				int DSDT_len = *(facp->DSDT + 1) - 36;

				while(DSDT_len > 0){
					if(memcmp(S5_addr, "_S5_", 4)){
						break;
					}
					S5_addr++;
				}

				if(DSDT_len > 0){
					if(( *(S5_addr-1) == 0x08 || (*(S5_addr - 2) == 0x008 && *(S5_addr - 1) == '\\') ) && *(S5_addr + 4) == 0x12){
						parsing_s5_obj(S5_addr, facp);
						printf("ACPI initialized\n");
						return;
					}
					else {
						printf("error while parsing _S5_ obj\n");
					}
				}
				else {
					printf("_S5_ obj not found\n");
				}
			}
			else {
				printf("DSDT invalid\n");
			}
		}
		rsdt++;
	}
	printf("no valid FACP\n");
}

uint8_t ACPI_enable(){
	if((i686_inw(PM1a_CNT_BLK) & SCI_EN) == 0){

		if(SMI_CMD != 0 && ACPI_ENABLE != 0){
			i686_outb(SMI_CMD, ACPI_ENABLE);
			int i = 0;
			for(; i < 3000; i++){
				if((i686_inw(PM1a_CNT_BLK) & SCI_EN) == 1){
					break;
				}
				i686_iowait();
			}

			if(PM1b_CNT_BLK != 0){
				for(; i < 3000; i++){
					if((i686_inw(PM1b_CNT_BLK) & SCI_EN) == 1){
						break;
					}
					i686_iowait();
				}
			}

			if(i < 3000){
				printf("ACPI Enabled\n");
				return 1;
			}
			else {
				printf("Failed to enable ACPI");
				return 0;
			}
		}
	}
	return 0;
}

void ACPI_poweroff(){
	if(SCI_EN == 0){
		return;
	}
	
	ACPI_enable();

	i686_outw(PM1a_CNT_BLK, SLP_TYPa | SLP_EN);
	if(PM1b_CNT_BLK != 0){
		i686_outw(PM1b_CNT_BLK, SLP_TYPb | SLP_EN);
	}
	printf("acpi poweroff Failed");
}
