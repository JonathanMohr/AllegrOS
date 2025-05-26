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
static VirtualAllocator g_VirtualAllocator;
static HeapAllocator g_HeapAllocator;

void memory_Initialize(MemoryInfo* memInfo, uint32_t kernelSize)
{
    PageDirectory pageDirectory = getPageDirectory();
    g_MemoryInfo = memInfo;
    PhysicalAllocator_Initialize(&g_PhysicalAllocator, memInfo);
    VirtualAllocator_Initialize(&g_VirtualAllocator, 0xC0000000, 0x40000000 - 0x00400000);
    g_VirtualAllocator.used = kernelSize;
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



void* page_Allocate(uint32_t pages)
{
    uint64_t alloc_size = pages * PAGE_SIZE;

    void* virtual = VirtualAllocator_Alloc(&g_VirtualAllocator, alloc_size, PAGE_SIZE);
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

        if (!Paging_Map((uintptr_t)virtual + i * PAGE_SIZE, (uintptr_t)physical))
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
    return PhysicalAllocator_Alloc(&g_PhysicalAllocator, size, align);
}

void memory_physicalFree(void* ptr)
{
    PhysicalAllocator_Free(&g_PhysicalAllocator, ptr);
}

void* memory_virtualAllocate(uint64_t size, uint64_t align)
{
    return VirtualAllocator_Alloc(&g_VirtualAllocator, size, align);
}

void memory_virtualFree(void* ptr)
{
    VirtualAllocator_Free(&g_VirtualAllocator, ptr);
}