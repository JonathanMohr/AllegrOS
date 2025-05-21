#include "physicalAllocator.h"

#include <stddef.h>

void PhysicalAllocator_Initialize(PhysicalAllocator* alloc, uint64_t base, uint64_t size)
{
    alloc->base = base;
    alloc->size = size;
    alloc->used = 0;
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