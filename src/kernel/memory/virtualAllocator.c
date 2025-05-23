#include "virtualAllocator.h"

#include <stddef.h>

void VirtualAllocator_Initialize(VirtualAllocator* alloc, uint64_t size)
{
    alloc->base = 0xC0000000;
    alloc->size = size;
    alloc->used = 0;
}

void VirtualAllocator_Skip(VirtualAllocator* alloc, uint64_t size)
{
    alloc->used += size;
}

void* VirtualAllocator_Alloc(VirtualAllocator* alloc, uint64_t size, uint64_t align)
{
    uintptr_t aligned = (alloc->base + alloc->used + (align - 1)) & ~(align - 1);
    uint64_t next_used = aligned - alloc->base + size;

    if (next_used > alloc->size) {
        return NULL; // Out of memory
    }

    alloc->used = next_used;
    return (void*)aligned;
}

void VirtualAllocator_Free(VirtualAllocator* alloc, void* ptr)
{
    //TODO
}