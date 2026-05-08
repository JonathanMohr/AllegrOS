#include "heap.h"

#include "physical/manager.h"

#include "../arch/x86/paging.h"

#include "../libc/memory.h"

void Memory_Heap_Initialize(Memory_Heap* heap, Memory_VirtualAllocator* virtualAllocator, uint64_t pageSize)
{
    heap->virtualAllocator = virtualAllocator;
    heap->head = NULL;
    heap->pageSize = pageSize;
}

Memory_Heap_BlockHeader* Memory_Heap_Expand(Memory_Heap* heap, uint64_t minSize)
{
    uint64_t needed = minSize + sizeof(Memory_Heap_BlockHeader);
    uint64_t pageCount = (needed + heap->pageSize - 1) / heap->pageSize;

    uintptr_t virtBase = Memory_VirtualAllocator_AllocatePage(heap->virtualAllocator, pageCount);
    if (virtBase == MEMORY_VIRTUALALLOCATOR_INVALID)
        return NULL;

    for (uint64_t i = 0; i < pageCount; i++)
    {
        uintptr_t phys = Memory_PhysicalAllocator_AllocatePage();
        if (phys == MEMORY_PHYSICALALLOCATOR_INVALID)
        {
            // TODO: Free already allocated physical pages
            for (uint64_t j = 0; j < i; j++)
                x86_PageDirectory_Unmap(virtBase + j * heap->pageSize);
            Memory_VirtualAllocator_FreePage(heap->virtualAllocator, virtBase);
            return NULL;
        }

        if (!x86_PageDirectory_Map(virtBase + i * heap->pageSize, phys))
        {
            // TODO: Free already allocated physical pages
            Memory_PhysicalAllocator_FreePage(phys);
            for (uint64_t j = 0; j < i; j++)
                x86_PageDirectory_Unmap(virtBase + j * heap->pageSize);
            Memory_VirtualAllocator_FreePage(heap->virtualAllocator, virtBase);
            return NULL;
        }
    }

    Memory_Heap_BlockHeader* block = (Memory_Heap_BlockHeader*)virtBase;
    block->size = pageCount * heap->pageSize - sizeof(Memory_Heap_BlockHeader);
    block->free = true;
    block->next = NULL;

    if (heap->head == NULL)
    {
        heap->head = block;
    }
    else
    {
        Memory_Heap_BlockHeader* cur = heap->head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = block;
    }

    return block;
}

void* Memory_Heap_Allocate(Memory_Heap* heap, uint64_t size)
{
    if (size == 0) return NULL;

    // Align to 8 byte
    size = (size + 7) & ~(uint64_t)7;

    Memory_Heap_BlockHeader* cur = heap->head;
    while (cur != NULL)
    {
        if (cur->free && cur->size >= size)
            break;
        cur = cur->next;
    }

    if (cur == NULL)
    {
        cur = Memory_Heap_Expand(heap, size);
        if (cur == NULL) return NULL;
    }

    if (cur->size >= (size + sizeof(Memory_Heap_BlockHeader) + 8))
    {
        Memory_Heap_BlockHeader* split = (Memory_Heap_BlockHeader*)((uintptr_t)cur + sizeof(Memory_Heap_BlockHeader) + size);
        split->size = cur->size - size - sizeof(Memory_Heap_BlockHeader);
        split->free = true;
        split->next = cur->next;

        cur->size = size;
        cur->next = split;
    }

    cur->free = false;
    return (void*)((uintptr_t)cur + sizeof(Memory_Heap_BlockHeader));
}

void Memory_Heap_Free(Memory_Heap* heap, void* ptr)
{
    if (ptr == NULL) return;

    Memory_Heap_BlockHeader* block = (Memory_Heap_BlockHeader*)((uintptr_t)ptr - sizeof(Memory_Heap_BlockHeader));
    block->free = true;

    Memory_Heap_BlockHeader* cur = heap->head;
    while (cur != NULL)
    {
        if (cur->free && cur->next != NULL && cur->next->free)
        {
            cur->size += sizeof(Memory_Heap_BlockHeader) + cur->next->size;
            cur->next = cur->next->next;
        }
        else
        {
            cur = cur->next;
        }
    }
}

void* Memory_Heap_Reallocate(Memory_Heap* heap, void* ptr, uint64_t newSize)
{
    if (ptr == NULL)
        return Memory_Heap_Allocate(heap, newSize);

    if (newSize == 0)
    {
        Memory_Heap_Free(heap, ptr);
        return NULL;
    }

    Memory_Heap_BlockHeader* block = (Memory_Heap_BlockHeader*)((uintptr_t)ptr - sizeof(Memory_Heap_BlockHeader));

    if (block->size >= newSize)
        return ptr;

    void* newPtr = Memory_Heap_Allocate(heap, newSize);
    if (newPtr == NULL)
        return NULL;

    memcpy(newPtr, ptr, block->size);

    Memory_Heap_Free(heap, ptr);
    return newPtr;
}
