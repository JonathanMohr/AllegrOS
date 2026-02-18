#include "memdetect.h"

#include "../x86/x86.h"

// TODO: Make dynamic
MemoryRegion memoryRegions[MAX_REGIONS];

void Memory_Detect(MemoryInfo* memoryInfo)
{
    x86_E820MemoryBlock block;
    uint32_t memoryRegionCount = 0;
    uint32_t continuation = 0;

    int ret = x86_E820GetNextBlock(&block, &continuation);

    while (ret > 0 && continuation != 0)
    {
        // TODO: I don't like it, it's very hardcoded
        if (
            memoryRegionCount > 0 &&
            ((memoryRegions[memoryRegionCount - 1].Begin + memoryRegions[memoryRegionCount - 1].Length) == 0xa0000) &&
            memoryRegions[memoryRegionCount].Begin != 0xa0000)
        {
            memoryRegions[memoryRegionCount].Begin = memoryRegions[memoryRegionCount - 1].Begin + memoryRegions[memoryRegionCount - 1].Length;
            memoryRegions[memoryRegionCount].Length = block.Base - memoryRegions[memoryRegionCount].Begin;
            memoryRegions[memoryRegionCount].Type = MEMORY_TYPE_HARDWARE;
            memoryRegions[memoryRegionCount].ACPI = 0;
            memoryRegionCount++;
        }

        memoryRegions[memoryRegionCount].Begin = block.Base;
        memoryRegions[memoryRegionCount].Length = block.Length;
        memoryRegions[memoryRegionCount].Type = block.Type;
        memoryRegions[memoryRegionCount].ACPI = block.ACPI;

        if (block.Base == 0) memoryRegions[memoryRegionCount].Type = MEMORY_TYPE_RELUCTANT;

        memoryRegionCount++;

        ret = x86_E820GetNextBlock(&block, &continuation);
    }

    memoryInfo->RegionCount = memoryRegionCount;
    memoryInfo->Regions = memoryRegions;
}
