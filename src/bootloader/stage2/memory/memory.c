#include "memory.h"

#include <core/memory/memory.h>

void Memory_AddBootRegion(MemoryInfo* memInfo)
{
    memset(newRegions, 0, sizeof(newRegions));
    int newCount = 0;

    for (int i = 0; i < memInfo->RegionCount; ++i)
    {
        MemoryRegion* region = &memInfo->Regions[i];
        uint64_t begin = region->Begin;
        uint64_t end = region->Begin + region->Length;

        // BOOT
        if (begin == 0)
        {
            MemoryRegion bootRegion = *region;
            bootRegion.Type = MEMORY_TYPE_BOOT;
            newRegions[newCount++] = bootRegion;
            continue;
        }

        newRegions[newCount++] = *region;
    }

    memInfo->RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        memInfo->Regions[i] = newRegions[i];
    }
}