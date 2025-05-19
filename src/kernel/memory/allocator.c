#include "allocator.h"

#include <stddef.h>
#include "../debug.h"

void BumpAllocator_Initialize(BumpAllocator* alloc, uint64_t base, uint64_t size)
{
    alloc->base = base;
    alloc->size = size;
    alloc->used = 0;
}

void* BumpAllocator_Alloc(BumpAllocator* alloc, uint64_t size, uint64_t align)
{
    uintptr_t aligned = (alloc->base + alloc->used + (align - 1)) & ~(align - 1);
    uint64_t next_used = aligned - alloc->base + size;

    if (next_used > alloc->size) {
        return NULL; // Out of memory
    }

    alloc->used = next_used;
    return (void*)aligned;
}