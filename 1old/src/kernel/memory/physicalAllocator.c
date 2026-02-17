#include "physicalAllocator.h"

#include <stddef.h>
#include "../debug.h"

void PhysicalAllocator_Initialize(PhysicalAllocator* alloc, MemoryInfo* memInfo)
{
    alloc->base = 0;
    alloc->size = 0;
    alloc->used = 0;
    for (uint32_t i = 0; i < memInfo->RegionCount; i++)
    {
        MemoryRegion* memRegion = &memInfo->Regions[i];
        if (memRegion->Type == MEMORY_TYPE_USABLE)
        {
            alloc->base = memRegion->Begin;
            alloc->size = memRegion->Length;
            break;
        }
    }
}

void* PhysicalAllocator_Alloc(PhysicalAllocator* alloc, uint64_t size, uint64_t align)
{
    uintptr_t aligned = (alloc->base + alloc->used + (align - 1)) & ~(align - 1);
    uint64_t next_used = aligned - alloc->base + size;

    if (next_used > alloc->size) {
        return NULL; // Out of memory
    }

    alloc->used = next_used;
    return (void*)aligned;
}

void PhysicalAllocator_Free(PhysicalAllocator* alloc, void* ptr)
{
    //TODO
}