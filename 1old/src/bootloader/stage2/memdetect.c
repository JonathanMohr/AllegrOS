#include "memdetect.h"

#include "stdio.h"
#include "x86.h"

MemoryRegion g_MemRegions[MAX_REGIONS];
int g_MemRegionCount;

void Memory_Detect(MemoryInfo* memoryInfo)
{
    E820MemoryBlock block;
    uint32_t continuation = 0;
    int ret;

    g_MemRegionCount = 0;
    ret = x86_E820GetNextBlock(&block, &continuation);

    while (ret > 0 && continuation != 0)
    {
        g_MemRegions[g_MemRegionCount].Begin = block.Base;
        g_MemRegions[g_MemRegionCount].Length = block.Length;
        g_MemRegions[g_MemRegionCount].Type = block.Type;
        g_MemRegions[g_MemRegionCount].ACPI = block.ACPI;
        ++g_MemRegionCount;

        ret = x86_E820GetNextBlock(&block, &continuation);
    }

    // fill meminfo structure
    memoryInfo->RegionCount = g_MemRegionCount;
    memoryInfo->Regions = g_MemRegions;
}