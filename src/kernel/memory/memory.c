#include "memory.h"

#include "physical/manager.h"
#include "virtual/manager.h"
#include "heap.h"

Memory_VirtualAllocator kernelSpaceAllocator;
Memory_Heap kernelHeap;

extern uint8_t __end[];

bool Memory_Initialize(MemoryInfo* memoryInfo)
{
    const uint64_t pageSize = 4096;

    if (!Memory_PhysicalAllocator_Initialize(memoryInfo, pageSize))
        return false;

    uintptr_t freeStart = (uintptr_t)&__end + 0x10000000;
    uintptr_t freeEnd = 0xFFC00000;

    if (!Memory_VirtualAllocator_Initialize(&kernelSpaceAllocator, pageSize, freeStart, freeEnd))
        return false;

    Memory_Heap_Initialize(&kernelHeap, &kernelSpaceAllocator, pageSize);

    return true;
}

void* Memory_KernelAllocate(uint64_t size)
{
    return Memory_Heap_Allocate(&kernelHeap, size);
}

void Memory_KernelFree(void* ptr)
{
    Memory_Heap_Free(&kernelHeap, ptr);
}

void* Memory_KernelReallocate(void* ptr, uint64_t newSize)
{
    return Memory_Heap_Reallocate(&kernelHeap, ptr, newSize);
}
