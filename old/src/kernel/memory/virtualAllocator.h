#pragma once

#include <stdint.h>

typedef struct {
    uintptr_t base;
    uint64_t size;
    uint64_t used;
} VirtualAllocator;

void VirtualAllocator_Initialize(VirtualAllocator* alloc, uint64_t base, uint64_t size);
void* VirtualAllocator_Alloc(VirtualAllocator* alloc, uint64_t size, uint64_t align);
void VirtualAllocator_Free(VirtualAllocator* alloc, void* ptr);