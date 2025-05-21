#include "heapAllocator.h"

#include <stddef.h>
#include "memory.h"

//TODO: remove
#include "../arch/i686/paging.h"

bool HeapAllocator_Initialize(PageDirectory* page_directory, HeapAllocator* alloc)
{
    alloc->start = (uintptr_t)memory_PageAllocate(page_directory, PAGE_SIZE);
    if (!alloc->start)
        return false;

    alloc->size = PAGE_SIZE;
    alloc->offset = 0;
    return true;
}

void* HeapAllocator_Alloc(PageDirectory* page_directory, HeapAllocator* alloc, uint64_t size, uint64_t align)
{
    uintptr_t current_ptr = (uintptr_t)(alloc->start + alloc->offset);

    uintptr_t aligned_ptr = (current_ptr + align - 1) & ~(align - 1);

    uint64_t padding = aligned_ptr - current_ptr;

    while (alloc->offset + padding + size > alloc->size) {
        void* new_page = memory_PageAllocate(page_directory, PAGE_SIZE);
        if (!new_page)
            return NULL;
        
        alloc->size += PAGE_SIZE;
    }

    void* result = (void*)aligned_ptr;

    // Offset für nächsten Alloc erhöhen
    alloc->offset += padding + size;

    return result;
}

void HeapAllocator_Free(PageDirectory* page_directory, HeapAllocator* alloc, uintptr_t ptr)
{
    // TODO
}