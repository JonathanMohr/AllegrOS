#include "memory.h"

#include "physical/manager.h"

bool Memory_Initialize(MemoryInfo* memoryInfo)
{
    const uint64_t pageSize = 4096;

    if (!Memory_PhysicalAllocator_Initialize(memoryInfo, 4096))
        return false;

    return true;
}
