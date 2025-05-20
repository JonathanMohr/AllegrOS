#pragma once

#include <boot/bootparams.h>

#define MEMORY_TYPE_USABLE              1
#define MEMORY_TYPE_RESERVED            2
#define MEMORY_TYPE_ACPI_RECLAIMABLE    3
#define MEMORY_TYPE_ACPI_NVS            4
#define MEMORY_TYPE_BAD                 5

#define MEMORY_TYPE_KERNEL              6
#define MEMORY_TYPE_BOOT                7

void memory_Initialize(MemoryInfo* memInfo, uintptr_t kernel_start, uintptr_t kernel_end);
void memory_Initialize_Allocator();

void* memory_physicalAllocate(uint64_t size, uint64_t align);
void memory_physicalFree(uintptr_t ptr);
void* memory_virtualAllocate(uint64_t size, uint64_t align);
void memory_virtualFree(uintptr_t ptr);
void* memory_Allocate(uint64_t size);
void memory_Free(uintptr_t ptr);

void* memory_ReserveRegionAndGetPtr(uint64_t size, uint64_t align);