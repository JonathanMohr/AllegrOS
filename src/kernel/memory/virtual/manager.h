#pragma once

#include <bootparams.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Memory_VirtualAllocator {
    uintptr_t base;
    uintptr_t size;
    uintptr_t used;

    uint64_t pageSize;

    bool initialized;
} Memory_VirtualAllocator;

bool Memory_VirtualAllocator_Initialize(Memory_VirtualAllocator* virtualAllocator, uint64_t pageSize, uintptr_t start, uintptr_t end);

#define MEMORY_VIRTUALALLOCATOR_INVALID ((uintptr_t)0)
uintptr_t Memory_VirtualAllocator_AllocatePage(Memory_VirtualAllocator* virtualAllocator);
void Memory_VirtualAllocator_FreePage(Memory_VirtualAllocator* virtualAllocator, uintptr_t page);
