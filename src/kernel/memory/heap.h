#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "virtual/manager.h"

typedef struct Memory_Heap_BlockHeader
{
    uint64_t size;
    struct Memory_Heap_BlockHeader* next;
    bool free;
} Memory_Heap_BlockHeader;

typedef struct Memory_Heap
{
    Memory_VirtualAllocator* virtualAllocator;
    Memory_Heap_BlockHeader* head;
    uint64_t pageSize;
} Memory_Heap;

void Memory_Heap_Initialize(Memory_Heap* heap, Memory_VirtualAllocator* virtualAllocator, uint64_t pageSize);
Memory_Heap_BlockHeader* Memory_Heap_Expand(Memory_Heap* heap, uint64_t minSize);

void* Memory_Heap_Allocate(Memory_Heap* heap, uint64_t size);
void Memory_Heap_Free(Memory_Heap* heap, void* ptr);
void* Memory_Heap_Reallocate(Memory_Heap* heap, void* ptr, uint64_t newSize);
