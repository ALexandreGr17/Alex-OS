#include "memdetect.h"
#include "boot/bootparams.h"
#include "x86.h"
#include <stdint.h>
#include "./stdio.h"


#define MAX_REGION 256
memory_region_t g_MemRegions[MAX_REGION];
int g_MemRegionsCount = 0;

void Memory_detect(memory_info_t* memoryInfo){
	E820MemoryBlock block;
	uint32_t continuation_id = 0;
	int ret;

	ret = x86_E820GetNextBlock(&block, &continuation_id);
	//printf("%i\n", ret);
	while(ret > 0 && continuation_id != 0){
		g_MemRegions[g_MemRegionsCount].Begin = block.Base;
		g_MemRegions[g_MemRegionsCount].Length = block.Length;
		g_MemRegions[g_MemRegionsCount].Type = block.Type;
		g_MemRegions[g_MemRegionsCount].ACPI = block.ACPI;
		g_MemRegionsCount++;
		
		printf("E820: base=0x%llx, length=0x%llx, type=0x%x\n", block.Base, block.Length, block.Type);
		ret = x86_E820GetNextBlock(&block, &continuation_id);
	}
	memoryInfo->region_count = g_MemRegionsCount;
	memoryInfo->regions = g_MemRegions;
}


