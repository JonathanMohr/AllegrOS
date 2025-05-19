#include "memory.h"

#include "physicalAllocator.h"
#include "../debug.h"

#include <core/memory/memory.h>

#include <stdbool.h>
#include <stddef.h>

static MemoryInfo* g_MemoryInfo;
static PhysicalAllocator g_PhysicalAllocator;

static MemoryRegion newRegions[MAX_REGIONS];

void memory_SanitizeMap(MemoryInfo* memInfo, uint64_t kernelStart, uint64_t kernelEnd) {
    memset(newRegions, 0, sizeof(newRegions));
    int newCount = 0;

    for (int i = 0; i < memInfo->RegionCount; ++i)
    {
        MemoryRegion* region = &memInfo->Regions[i];
        uint64_t begin = region->Begin;
        uint64_t end = region->Begin + region->Length;

        // BOOT
        if (begin == 0)
        {
            MemoryRegion bootRegion = *region;
            bootRegion.Type = MEMORY_TYPE_BOOT;
            newRegions[newCount++] = bootRegion;
            continue;
        }

        // KERNEL
        if (end <= kernelStart || begin >= kernelEnd)
        {
            newRegions[newCount++] = *region;
            continue;
        }

        if (begin >= kernelStart && end <= kernelEnd)
        {
            MemoryRegion kernel = *region;
            kernel.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernel;
            continue;
        }

        if (begin < kernelStart && end > kernelStart && end <= kernelEnd)
        {
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

        if (begin >= kernelStart && begin < kernelEnd && end > kernelEnd)
        {
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

        if (begin < kernelStart && end > kernelEnd)
        {
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
}

void memory_Initialize_Allocator()
{
    bool ok = false;

    for (int i = 0; i < g_MemoryInfo->RegionCount; i++)
    {
        MemoryRegion* region = &g_MemoryInfo->Regions[i];
        if (region->Type == MEMORY_TYPE_USABLE && region->Length >= 4096)
        {
            PhysicalAllocator_Initialize(&g_PhysicalAllocator, region->Begin, region->Length);
            log_info("Physical Allocator", "Initialized at 0x%llx with size %llu\n", region->Begin, region->Length);
            ok = true;
        }
    }
}

void* memory_Allocate(uint64_t size, uint64_t align)
{
    //TODO
    
    //return PhysicalAllocator_Alloc(&g_PhysicalAllocator, size, align);
}

void memory_Free(uintptr_t ptr)
{
    //TODO
}

static inline uint64_t align_up(uint64_t addr, uint64_t align)
{
    return (addr + align - 1) & ~(align - 1);
}

void* memory_ReserveRegionAndGetPtr(uint64_t size, uint64_t align)
{
    memset(newRegions, 0, sizeof(newRegions));
    int newCount = 0;

    void* ptr = NULL;

    for (int i = 0; i < g_MemoryInfo->RegionCount; ++i)
    {
        MemoryRegion region = g_MemoryInfo->Regions[i];

        if (region.Type == MEMORY_TYPE_USABLE)
        {
            uint64_t alignedBegin = align_up(region.Begin, align);
            uint64_t padding = alignedBegin - region.Begin;

            if (region.Length >= size + padding && ptr == NULL)
            {
                if (padding > 0)
                {
                    MemoryRegion before = region;
                    before.Length = padding;
                    newRegions[newCount++] = before;
                }

                MemoryRegion reserved = region;
                reserved.Begin = alignedBegin;
                reserved.Length = size;
                reserved.Type = MEMORY_TYPE_RESERVED;
                newRegions[newCount++] = reserved;

                ptr = (void*)(uintptr_t)alignedBegin;

                uint64_t afterBegin = alignedBegin + size;
                uint64_t afterLength = (region.Begin + region.Length) - afterBegin;
                if (afterLength > 0)
                {
                    region.Begin = afterBegin;
                    region.Length = afterLength;
                }
            }
        }

        newRegions[newCount++] = region;
    }

    g_MemoryInfo->RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        g_MemoryInfo->Regions[i] = newRegions[i];
    }

    return ptr;
}