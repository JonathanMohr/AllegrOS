#pragma once

#include <boot/bootparams.h>

#define MEMORY_TYPE_USABLE              1
#define MEMORY_TYPE_RESERVED            2
#define MEMORY_TYPE_ACPI_RECLAIMABLE    3
#define MEMORY_TYPE_ACPI_NVS            4
#define MEMORY_TYPE_BAD                 5

#define MEMORY_TYPE_KERNEL              6

void memory_Initialize(MemoryInfo* memInfo, uintptr_t kernel_start, uintptr_t kernel_end);
void* memory_Allocate(uint64_t size, uint64_t align);