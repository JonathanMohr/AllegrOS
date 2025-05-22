#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uintptr_t start;
    uint64_t size;
    uint64_t offset;
} HeapAllocator;

/*
bool HeapAllocator_Initialize(PageDirectory* page_directory, HeapAllocator* alloc);
void* HeapAllocator_Alloc(PageDirectory* page_directory, HeapAllocator* alloc, uint64_t size, uint64_t align);
void HeapAllocator_Free(PageDirectory* page_directory, HeapAllocator* alloc, uintptr_t ptr);
*/