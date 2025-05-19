#pragma once

#include <stdint.h>

typedef struct {
    uintptr_t base;
    uint64_t size;
    uint64_t used;
} BumpAllocator;

void BumpAllocator_Initialize(BumpAllocator* alloc, uint64_t base, uint64_t size);
void* BumpAllocator_Alloc(BumpAllocator* alloc, uint64_t size, uint64_t align);