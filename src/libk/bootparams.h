#pragma once

#include <stdint.h>

#define MAX_REGIONS 256

#define MEMORY_TYPE_USABLE              1
#define MEMORY_TYPE_RESERVED            2
#define MEMORY_TYPE_ACPI_RECLAIMABLE    3
#define MEMORY_TYPE_ACPI_NVS            4
#define MEMORY_TYPE_BAD                 5

#define MEMORY_TYPE_RELUCTANT           6
#define MEMORY_TYPE_HARDWARE            7
#define MEMORY_TYPE_KERNEL_PAGEINFO     8
#define MEMORY_TYPE_KERNEL_PAGETABLE    9
#define MEMORY_TYPE_KERNEL              10

typedef struct MemoryRegion {
    uint64_t Begin, Length;
    uint32_t Type;
    uint32_t ACPI;
} MemoryRegion;

typedef struct MemoryInfo {
    uint32_t RegionCount;
    MemoryRegion Regions[MAX_REGIONS];

    void* physPageArray;
    void* startOfFree;

    uphysptr_t initialAddressSpace;
} MemoryInfo;

typedef struct BootParams {
    MemoryInfo Memory;
    uint8_t BootDevice;
} BootParams;
