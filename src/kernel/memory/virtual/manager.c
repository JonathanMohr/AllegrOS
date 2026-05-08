#include "manager.h"

bool Memory_VirtualAllocator_Initialize(Memory_VirtualAllocator* virtualAllocator, uint64_t pageSize, uintptr_t start, uintptr_t end)
{
    virtualAllocator->initialized = false;

    uint64_t alignedBegin = ((start + (pageSize - 1)) / pageSize) * pageSize;
    if (alignedBegin >= end)
        return false;

    uint64_t adjustedSize = end - alignedBegin;
    if (adjustedSize < pageSize)
        return false;

    virtualAllocator->initialized = true;
    virtualAllocator->base = alignedBegin;
    virtualAllocator->size = adjustedSize;
    virtualAllocator->used = 0;

    virtualAllocator->pageSize = pageSize;

    return true;
}

uintptr_t Memory_VirtualAllocator_AllocatePage(Memory_VirtualAllocator* virtualAllocator, uint64_t count)
{
    if (!virtualAllocator->initialized) return MEMORY_VIRTUALALLOCATOR_INVALID;

    if ((virtualAllocator->size - virtualAllocator->used) < (virtualAllocator->pageSize * count))
        return MEMORY_VIRTUALALLOCATOR_INVALID; // Out of space

    const uintptr_t address = virtualAllocator->base + virtualAllocator->used;
    virtualAllocator->used += virtualAllocator->pageSize * count;
    return address;
}

void Memory_VirtualAllocator_FreePage(Memory_VirtualAllocator* virtualAllocator, uintptr_t page)
{
    // TODO
}
