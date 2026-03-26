#include "manager.h"

Memory_PhysicalAllocator physicalAllocator = {
    .initialized = false
};

bool Memory_PhysicalRegionManager_Initialize(Memory_PhysicalRegionManager* regionManager, MemoryRegion* region, uint64_t pageSize)
{
    regionManager->initialized = false;

    if (region->Type != MEMORY_TYPE_USABLE)
        return false;

    uint64_t alignedBegin = ((region->Begin + (pageSize - 1)) / pageSize) * pageSize;
    if (alignedBegin >= (region->Begin + region->Length))
        return false;

    uint64_t adjustedSize = region->Begin + region->Length - alignedBegin;
    if (adjustedSize < pageSize)
        return false;
    
    regionManager->initialized = true;
    regionManager->base = (uintptr_t)alignedBegin;
    regionManager->size = (uintptr_t)adjustedSize;
    regionManager->used = 0;

    return true;
}

bool Memory_PhysicalRegionManager_IsPart(Memory_PhysicalRegionManager* regionManager, uint64_t pageSize, uintptr_t page)
{
    if (!regionManager->initialized) return false;

    uintptr_t start = regionManager->base;
    uintptr_t end = regionManager->base + regionManager->used;

    if (page < start)
        return false;

    if (page >= end)
        return false;

    if (page % pageSize != 0)
        return false;

    return true;
}

uintptr_t Memory_PhysicalRegionManager_AllocatePage(Memory_PhysicalRegionManager* regionManager, uint64_t pageSize)
{
    if (!regionManager->initialized) return MEMORY_PHYSICALALLOCATOR_INVALID;

    if ((regionManager->size - regionManager->used) < pageSize)
        return MEMORY_PHYSICALALLOCATOR_INVALID; // Out of memory

    regionManager->used += pageSize;
    return regionManager->base + regionManager->used - pageSize;
}

void Memory_PhysicalRegionManager_Free(Memory_PhysicalRegionManager* regionManager, uint64_t pageSize, uintptr_t page)
{
    // TODO
}


bool Memory_PhysicalAllocator_Initialize(MemoryInfo* memInfo, uint64_t pageSize)
{
    if (physicalAllocator.initialized) return false;
    physicalAllocator.initialized = false;

    bool found = false;
    for (uint32_t i = 0; i < memInfo->RegionCount; i++)
    {
        MemoryRegion* region = &memInfo->Regions[i];
        if (region->Type == MEMORY_TYPE_USABLE)
        {
            if (Memory_PhysicalRegionManager_Initialize(&physicalAllocator.region, region, pageSize))
            {
                found = true;
                break;
            }
        }
    }
    if (!found) return false;

    physicalAllocator.initialized = true;
    physicalAllocator.pageSize = pageSize;

    return found;
}

uintptr_t Memory_PhysicalAllocator_AllocatePage()
{
    if (!physicalAllocator.initialized) return MEMORY_PHYSICALALLOCATOR_INVALID;

    return Memory_PhysicalRegionManager_AllocatePage(&physicalAllocator.region, physicalAllocator.pageSize);
}

void Memory_PhysicalAllocator_FreePage(uintptr_t page)
{
    if (!physicalAllocator.initialized) return;

    Memory_PhysicalRegionManager_Free(&physicalAllocator.region, physicalAllocator.pageSize, page);
}
