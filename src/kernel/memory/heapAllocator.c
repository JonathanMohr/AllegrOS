#include "heapAllocator.h"

#include <stddef.h>
#include "memory.h"

bool HeapAllocator_Initialize(HeapAllocator* alloc)
{
    alloc->start = (uintptr_t)page_Allocate(1);
    if (alloc->start)
        return false;

    alloc->size = PAGE_SIZE;
    alloc->offset = 0;
    return true;
}

void* HeapAllocator_Alloc(HeapAllocator* alloc, uint64_t size, uint64_t align)
{
    uintptr_t current_ptr = (uintptr_t)(alloc->start + alloc->offset);
    uintptr_t aligned_ptr = (current_ptr + align - 1) & ~(align - 1);
    uint64_t padding = aligned_ptr - current_ptr;

    while (alloc->offset + padding + size > alloc->size) {
        void* new_page = page_Allocate(1);
        if (!new_page)
            return NULL;
        alloc->size += PAGE_SIZE;
    }
    alloc->offset += padding + size;

    void* result = (void*)aligned_ptr;
    return result;
}

void HeapAllocator_Free(HeapAllocator* alloc, uintptr_t ptr)
{
    // TODO
}