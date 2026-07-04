#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct Block_Device
{
    char name[128];

    uint64_t sectorSize;
    uint64_t sectorCount;

    const char* type;
    
    uint64_t (*read)(struct Block_Device* dev, uint64_t sector, uint64_t count, void* buffer);
    uint64_t (*write)(struct Block_Device* dev, uint64_t sector, uint64_t count, const void* buffer);

    void (*destroy)(struct Block_Device* dev);

    void* data;
} Block_Device;

typedef struct Device_PartitionTable
{
    Block_Device* parent;

    uint64_t (*getPartitionCount)(struct Device_PartitionTable* table);
    /** outStart and outCount as sectors, not bytes */
    bool (*getPartitionEntry)(struct Device_PartitionTable* table, uint64_t index, uint64_t* outStart, uint64_t* outCount);

    void (*destroy)(struct Device_PartitionTable* table);

    void* data;
} Device_PartitionTable;

bool Device_PartitionTable_CreateBlockDevice(Block_Device* parent, Block_Device* device, const char* name, uint64_t start, uint64_t count);
