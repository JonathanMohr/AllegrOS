#include "memory.h"

#include "allocator.h"
#include "../debug.h"

#include <stdbool.h>

MemoryInfo* g_MemoryInfo;
BumpAllocator g_Allocator;

bool allocator_Initialize()
{
    for (int i = 0; i < g_MemoryInfo->RegionCount; i++)
    {
        MemoryRegion* region = &g_MemoryInfo->Regions[i];
        if (region->Type == MEMORY_TYPE_USABLE && region->Begin >= 0x100000)
        {
            BumpAllocator_Initialize(&g_Allocator, region->Begin, region->Length);
            log_info("Allocator", "Initialized at 0x%llx with size %llu\n", region->Begin, region->Length);
            return true;
        }
    }
    return false;
}

void memory_SanitizeMap(MemoryInfo* memInfo, uint64_t kernelStart, uint64_t kernelEnd) {
    static MemoryRegion newRegions[MAX_REGIONS];
    int newCount = 0;

    for (int i = 0; i < memInfo->RegionCount; ++i) {
        MemoryRegion* region = &memInfo->Regions[i];
        uint64_t begin = region->Begin;
        uint64_t end = region->Begin + region->Length;

        if (end <= kernelStart || begin >= kernelEnd) {
            newRegions[newCount++] = *region;
            continue;
        }

        if (begin >= kernelStart && end <= kernelEnd) {
            MemoryRegion kernel = *region;
            kernel.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernel;
            continue;
        }

        if (begin < kernelStart && end > kernelStart && end <= kernelEnd) {
            MemoryRegion before = *region;
            before.Length = kernelStart - begin;
            newRegions[newCount++] = before;

            MemoryRegion kernel = *region;
            kernel.Begin = kernelStart;
            kernel.Length = end - kernelStart;
            kernel.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernel;
            continue;
        }

        if (begin >= kernelStart && begin < kernelEnd && end > kernelEnd) {
            MemoryRegion kernel = *region;
            kernel.Length = kernelEnd - begin;
            kernel.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernel;

            MemoryRegion after = *region;
            after.Begin = kernelEnd;
            after.Length = end - kernelEnd;
            newRegions[newCount++] = after;
            continue;
        }

        if (begin < kernelStart && end > kernelEnd) {
            MemoryRegion before = *region;
            before.Length = kernelStart - begin;
            newRegions[newCount++] = before;

            MemoryRegion kernel = *region;
            kernel.Begin = kernelStart;
            kernel.Length = kernelEnd - kernelStart;
            kernel.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernel;

            MemoryRegion after = *region;
            after.Begin = kernelEnd;
            after.Length = end - kernelEnd;
            newRegions[newCount++] = after;
            continue;
        }
    }

    memInfo->RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        memInfo->Regions[i] = newRegions[i];
    }
}

void memory_Initialize(MemoryInfo* memInfo, uintptr_t kernel_start, uintptr_t kernel_end)
{
    g_MemoryInfo = memInfo;
    memory_SanitizeMap(g_MemoryInfo, kernel_start, kernel_end);

    bool result = allocator_Initialize();
    if (!result)
        log_crit("KERNEL | MEMORY", "Failed to initialize memory allocator!");
}

void* memory_Allocate(uint64_t size, uint64_t align)
{
    return BumpAllocator_Alloc(&g_Allocator, size, align);
}