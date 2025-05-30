#include "memory.h"

#include "physicalAllocator.h"
#include "virtualAllocator.h"
#include "../debug.h"
#include "heapAllocator.h"

#include <core/memory/memory.h>

#include <stddef.h>
#include "../debug.h"
#include "../hal/paging.h"

static MemoryInfo* g_MemoryInfo;
static PhysicalAllocator g_PhysicalAllocator;
static VirtualAllocator g_KernelVirtualAllocator;
static VirtualAllocator g_UserVirtualAllocator;
static HeapAllocator g_HeapAllocator;

void memory_Initialize(MemoryInfo* memInfo, uint32_t kernelSize)
{
    PageDirectory pageDirectory = getPageDirectory();
    g_MemoryInfo = memInfo;

    PhysicalAllocator_Initialize(&g_PhysicalAllocator, memInfo);

    VirtualAllocator_Initialize(&g_KernelVirtualAllocator, 0xC0000000, 0x40000000 - 0x00400000 + 1);
    g_KernelVirtualAllocator.used = kernelSize;

    // TODO: remove 0x00400000
    VirtualAllocator_Initialize(&g_UserVirtualAllocator, 0x00400000, 0xBFFFFFFF - 0x00400000 + 1);

    HeapAllocator_Initialize(&g_HeapAllocator);
}

void* memory_Allocate(uint64_t size, uint64_t align)
{
    return HeapAllocator_Alloc(&g_HeapAllocator, size, align);
}

void memory_Free(void* ptr)
{
    HeapAllocator_Free(&g_HeapAllocator, ptr);
}



void* specific_Allocate(uint32_t bytes, void* start, bool user)
{
    uintptr_t virtual_start = (uintptr_t)start;
    uint32_t padding = 0;

    if (virtual_start & (PAGE_SIZE - 1))
    {
        padding = virtual_start & (PAGE_SIZE - 1);
        virtual_start = virtual_start & ~(PAGE_SIZE - 1);
    }

    uint32_t pages = (bytes + padding + PAGE_SIZE - 1) / PAGE_SIZE;  // Auf ganze Seiten aufrunden

    for (uint32_t i = 0; i < pages; i++) {
        void* physical = PhysicalAllocator_Alloc(&g_PhysicalAllocator, PAGE_SIZE, PAGE_SIZE);
        if (!physical) {
            // TODO: Fehlerbehandlung (Rollback schon gemappter Seiten)
            return NULL;
        }

        if (!Paging_Map(virtual_start + i * PAGE_SIZE, (uintptr_t)physical, user))
        {
            // TODO: Fehlerbehandlung (Rollback)
            return NULL;
        }
    }

    return start;
}

void specific_Free(void* ptr)
{
    // TODO
}

void* page_Allocate(uint32_t pages)
{
    uint64_t alloc_size = pages * PAGE_SIZE;

    void* virtual = VirtualAllocator_Alloc(&g_KernelVirtualAllocator, alloc_size, PAGE_SIZE);
    if (!virtual)
    {
        // TODO
        return NULL;
    }

    for (uint64_t i = 0; i < pages; i++) {
        void* physical = PhysicalAllocator_Alloc(&g_PhysicalAllocator, PAGE_SIZE, PAGE_SIZE);
        if (!physical)
        {
            // TODO
            return NULL;
        }

        if (!Paging_Map((uintptr_t)virtual + i * PAGE_SIZE, (uintptr_t)physical, false))
        {
            // TODO
            return NULL;
        }

        //TODO: debug log print
        //log_debug("Memory", "Virtual: %p, Physical: %p", virtual + i * PAGE_SIZE, physical);
    }

    return virtual;
}

void page_Free(uintptr_t ptr)
{
    // TODO
}

void* memory_physicalAllocate(uint64_t size, uint64_t align, bool freeable)
{
    //TODO: freeable
    return PhysicalAllocator_Alloc(&g_PhysicalAllocator, size, align);
}

void memory_physicalFree(void* ptr)
{
    //TODO: freeable
    PhysicalAllocator_Free(&g_PhysicalAllocator, ptr);
}

void memory_physicalForceFree(void* ptr)
{
    //TODO
}

void* memory_virtualAllocate(uint64_t size, uint64_t align)
{
    return VirtualAllocator_Alloc(&g_KernelVirtualAllocator, size, align);
}

void memory_virtualFree(void* ptr)
{
    VirtualAllocator_Free(&g_KernelVirtualAllocator, ptr);
}


void* page_UserAllocate(uint32_t pages)
{
    uint64_t alloc_size = pages * PAGE_SIZE;

    void* virtual = VirtualAllocator_Alloc(&g_UserVirtualAllocator, alloc_size, PAGE_SIZE);
    if (!virtual)
    {
        // TODO
        return NULL;
    }

    for (uint64_t i = 0; i < pages; i++) {
        void* physical = PhysicalAllocator_Alloc(&g_PhysicalAllocator, PAGE_SIZE, PAGE_SIZE);
        if (!physical)
        {
            // TODO
            return NULL;
        }

        if (!Paging_Map((uintptr_t)virtual + i * PAGE_SIZE, (uintptr_t)physical, true))
        {
            // TODO
            return NULL;
        }

        //TODO: debug log print
        //log_debug("Memory", "Virtual: %p, Physical: %p", virtual + i * PAGE_SIZE, physical);
    }

    return virtual;
}

void page_UserFree(uintptr_t ptr)
{
    // TODO
}