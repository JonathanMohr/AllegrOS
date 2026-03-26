#pragma once

#include <bootparams.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Memory_PhysicalRegionManager {
    uintptr_t base;
    uintptr_t size;
    uintptr_t used;

    bool initialized;
} Memory_PhysicalRegionManager;

typedef struct Memory_PhysicalAllocator {
    Memory_PhysicalRegionManager region;

    uint64_t pageSize;

    bool initialized;

} Memory_PhysicalAllocator;

bool Memory_PhysicalAllocator_Initialize(MemoryInfo* memInfo, uint64_t pageSize);

#define MEMORY_PHYSICALALLOCATOR_INVALID ((uintptr_t)0)
uintptr_t Memory_PhysicalAllocator_AllocatePage();
void Memory_PhysicalAllocator_FreePage(uintptr_t page);
