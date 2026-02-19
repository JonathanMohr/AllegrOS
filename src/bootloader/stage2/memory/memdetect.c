#include "memdetect.h"

#include <bootparams.h>
#include "../io/io.h"
#include "../x86/x86.h"

void Memory_Detect(MemoryInfo* memoryInfo, x86_E820MemoryBlock* blocks, uint32_t count)
{
    for (uint32_t i = 0; i < count - 1; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (blocks[j].Base < blocks[i].Base)
            {
                x86_E820MemoryBlock tmp = blocks[i];
                blocks[i] = blocks[j];
                blocks[j] = tmp;
            }
        }
    }

    uint32_t regionCount = 0;
    uint64_t lastEnd = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        uint64_t base = blocks[i].Base;
        uint64_t length = blocks[i].Length;
        uint32_t type = blocks[i].Type;
        uint32_t acpi = blocks[i].ACPI;

        /*
        // Gap
        if (base > lastEnd)
        {
            memoryInfo->Regions[regionCount].Begin = lastEnd;
            memoryInfo->Regions[regionCount].Length = base - lastEnd;
            memoryInfo->Regions[regionCount].Type = MEMORY_TYPE_HARDWARE;
            memoryInfo->Regions[regionCount].ACPI = 0;
            regionCount++;
        }

        // Overlap
        if (base < lastEnd)
        {
            uint64_t overlap = lastEnd - base;
            base += overlap;
            if (length <= overlap) continue;
            length -= overlap;
        }
        */

        memoryInfo->Regions[regionCount].Begin = base;
        memoryInfo->Regions[regionCount].Length = length;
        memoryInfo->Regions[regionCount].Type = type;
        memoryInfo->Regions[regionCount].ACPI = acpi;

        lastEnd = base + length;
        regionCount++;
    }

    memoryInfo->RegionCount = regionCount;
}
