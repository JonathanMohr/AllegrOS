#include "memory.h"

#include "physicalAllocator.h"
#include "virtualAllocator.h"
#include "../debug.h"
#include "heapAllocator.h"

#include <core/memory/memory.h>

#include <stdbool.h>
#include <stddef.h>

static MemoryInfo* g_MemoryInfo;
static PhysicalAllocator g_PhysicalAllocator;
static VirtualAllocator g_VirtualAllocator;
static HeapAllocator g_HeapAllocator;

static MemoryRegion newRegions[MAX_REGIONS];

static PageDirectory* g_KernelPageDirectory;

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

void memory_Initialize_Allocator(PageDirectory* page_directory)
{
    bool ok = false;

    uint64_t heap_start;

    for (int i = 0; i < g_MemoryInfo->RegionCount; i++)
    {
        MemoryRegion* region = &g_MemoryInfo->Regions[i];
        if (region->Type == MEMORY_TYPE_USABLE && region->Length >= 4096)
        {
            heap_start = region->Begin;
            PhysicalAllocator_Initialize(&g_PhysicalAllocator, region->Begin, region->Length);
            //TODO: info log print
            log_info("Physical Allocator", "Initialized at 0x%llx with size %llu", region->Begin, region->Length);
            ok = true;
        }
    }

    VirtualAllocator_Initialize(&g_VirtualAllocator, 32329728);
    g_VirtualAllocator.base = heap_start;
    //TODO: info log print
    log_info("Virtual Allocator", "Initialized at 0x%lx (virtual) with size %llu", g_VirtualAllocator.base, g_VirtualAllocator.size);

    HeapAllocator_Initialize(page_directory, &g_HeapAllocator);
    g_KernelPageDirectory = page_directory;
}

void* memory_physicalAllocate(uint64_t size, uint64_t align)
{
    return PhysicalAllocator_Alloc(&g_PhysicalAllocator, size, align);
}

void memory_physicalFree(uintptr_t ptr)
{
    //TODO
}

void* memory_virtualAllocate(uint64_t size, uint64_t align)
{
    return VirtualAllocator_Alloc(&g_VirtualAllocator, size, align);
}

void memory_virtualFree(uintptr_t ptr)
{
    //TODO
}

//TODO:remove arch
#include "../arch/i686/paging.h"

void* memory_PageAllocate(PageDirectory* page_directory, uint64_t size)
{
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t alloc_size = pages * PAGE_SIZE;

    void* virtual = VirtualAllocator_Alloc(&g_VirtualAllocator, alloc_size, PAGE_SIZE);
    if (!virtual)
    {
        // TODO
        return NULL;
    }

    for (uint64_t i = 0; i < pages; i++) {
        void* physical = PhysicalAllocator_Alloc(&g_PhysicalAllocator, PAGE_SIZE, PAGE_SIZE);
        if (!physical) {
            // TODO
            return NULL;
        }

        bool ok = Paging_Map(page_directory, g_KernelPageDirectory, (uintptr_t)virtual + i * PAGE_SIZE, (uintptr_t)physical);
        if (!ok)
            // TODO
            return NULL;

        //TODO: debug log print
        // log_debug("Memory", "Virtual: %p, Physical: %p", virtual, physical);
    }

    return virtual;
}

void memory_PageFree(PageDirectory* page_directory, uintptr_t ptr)
{
    //TODO
}

void* memory_Allocate(uint64_t size, uint64_t align)
{
    return HeapAllocator_Alloc(g_KernelPageDirectory, &g_HeapAllocator, size, align);
}

void memory_Free(uintptr_t ptr)
{
    HeapAllocator_Free(g_KernelPageDirectory, &g_HeapAllocator, ptr);
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
                MemoryRegion reserved = region;
                reserved.Length = padding + size;
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