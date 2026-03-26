#include "memory.h"

#include "physical/manager.h"
#include "virtual/manager.h"

Memory_VirtualAllocator kernelSpaceAllocator;

extern uint8_t __end[];

bool Memory_Initialize(MemoryInfo* memoryInfo)
{
    const uint64_t pageSize = 4096;

    if (!Memory_PhysicalAllocator_Initialize(memoryInfo, pageSize))
        return false;

    uintptr_t freeStart = (uintptr_t)&__end;
    uintptr_t freeEnd = 0xFFC00000;

    if (!Memory_VirtualAllocator_Initialize(&kernelSpaceAllocator, pageSize, freeStart, freeEnd))
        return false;

    return true;
}
