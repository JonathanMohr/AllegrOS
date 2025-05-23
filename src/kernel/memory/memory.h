#pragma once

#include <boot/bootparams.h>
#include <boot/arch/i686/paging.h>


void memory_Initialize(MemoryInfo* memInfo, uint32_t kernelSize);

void* memory_Allocate(uint64_t size, uint64_t align);
void memory_Free(uintptr_t ptr);


void* page_Allocate(uint32_t pages);
void page_Free(uintptr_t ptr);