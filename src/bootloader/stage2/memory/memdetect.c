#include "memdetect.h"

#include "../io/io.h"
#include "../x86/x86.h"

void Memory_Detect(MemoryInfo* memoryInfo)
{
    x86_E820MemoryBlock blocks[MAX_REGIONS];
    uint32_t count = 0;
    uint32_t continuation = 0;

    int ret = x86_E820GetNextBlock(&blocks[count], &continuation);
    while (ret > 0)
    {
        if (count >= MAX_REGIONS)
        {
            IO_PutString(dbgout, "Warning: More regions than MAX_REGIONS!");
        }
        
        count++;
        ret = x86_E820GetNextBlock(&blocks[count], &continuation);

        if (continuation == 0) break;
    }

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

        memoryInfo->Regions[regionCount].Begin = base;
        memoryInfo->Regions[regionCount].Length = length;
        memoryInfo->Regions[regionCount].Type = type;
        memoryInfo->Regions[regionCount].ACPI = acpi;

        lastEnd = base + length;
        regionCount++;
    }

    memoryInfo->RegionCount = regionCount;
}
