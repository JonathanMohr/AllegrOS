#pragma once

#include <stdint.h>

#define MAX_REGIONS 256

#define MEMORY_TYPE_USABLE              1
#define MEMORY_TYPE_RESERVED            2
#define MEMORY_TYPE_ACPI_RECLAIMABLE    3
#define MEMORY_TYPE_ACPI_NVS            4
#define MEMORY_TYPE_BAD                 5

#define MEMORY_TYPE_KERNEL              6
#define MEMORY_TYPE_BOOT                7

typedef struct {
    uint64_t Begin, Length;
    uint32_t Type;
    uint32_t ACPI;
} MemoryRegion;

typedef struct {
    int RegionCount;
    MemoryRegion* Regions;
} MemoryInfo;

typedef struct {
    MemoryInfo Memory;
    uint8_t BootDevice;
    uint32_t kernelSize;
} BootParams;