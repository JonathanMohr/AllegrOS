#pragma once

#include <stdint.h>

typedef struct Block_Device
{
    char name[128];

    uint64_t sectorSize;
    uint64_t sectorCount;

    const char* type;
    
    uint64_t (*read)(struct Block_Device* dev, uint64_t sector, uint64_t count, void* buffer);
    uint64_t (*write)(struct Block_Device* dev, uint64_t sector, uint64_t count, const void* buffer);

    void* data;
} Block_Device;
