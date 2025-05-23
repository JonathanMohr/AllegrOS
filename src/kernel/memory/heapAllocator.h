#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <boot/arch/i686/paging.h>

typedef struct {
    uintptr_t start;
    uint64_t size;
    uint64_t offset;
} HeapAllocator;

bool HeapAllocator_Initialize(HeapAllocator* alloc);
void* HeapAllocator_Alloc(HeapAllocator* alloc, uint64_t size, uint64_t align);
void HeapAllocator_Free(HeapAllocator* alloc, uintptr_t ptr);