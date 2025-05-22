#pragma once

#include <boot/bootparams.h>


void memory_Initialize(MemoryInfo* memInfo);

void* memory_Allocate(uint64_t size, uint64_t align);
void memory_Free(uintptr_t ptr);

/*
void memory_Initialize_Allocator(PageDirectory* page_directory);

void* memory_physicalAllocate(uint64_t size, uint64_t align);
void memory_physicalFree(uintptr_t ptr);

void* memory_virtualAllocate(uint64_t size, uint64_t align);
void memory_virtualFree(uintptr_t ptr);

void* memory_PageAllocate(PageDirectory* page_directory, uint64_t size);
void memory_PageFree(PageDirectory* page_directory, uintptr_t ptr);



void* memory_ReserveRegionAndGetPtr(uint64_t size, uint64_t align);
*/