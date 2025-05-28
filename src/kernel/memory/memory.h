#pragma once

#include <boot/bootparams.h>
#include <boot/arch/i686/paging.h>
#include <stdbool.h>

void memory_Initialize(MemoryInfo* memInfo, uint32_t kernelSize);

void* memory_Allocate(uint64_t size, uint64_t align);
void memory_Free(void* ptr);


void* page_Allocate(uint32_t pages);
void page_Free(uintptr_t ptr);

void* memory_physicalAllocate(uint64_t size, uint64_t align, bool freeable);
void memory_physicalFree(void* ptr);
void memory_physicalForceFree(void* ptr);

void* memory_virtualAllocate(uint64_t size, uint64_t align);
void memory_virtualFree(void* ptr);


void* page_UserAllocate(uint32_t pages);
void page_UserFree(uintptr_t ptr);