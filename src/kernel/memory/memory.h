#pragma once

#include <bootparams.h>
#include <stdbool.h>

bool Memory_Initialize(MemoryInfo* memoryInfo);

void* Memory_KernelAllocate(uint64_t size);
void Memory_KernelFree(void* ptr);
void* Memory_KernelReallocate(void* ptr, uint64_t newSize);
