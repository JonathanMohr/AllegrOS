#include "mbr.h"

#include <memory.h>
#include "../../memory/memory.h"

typedef struct MBR_Partition
{
    uint32_t startLba;
    uint32_t sectorCount;

    uint8_t type;
    bool bootable;

    bool set;
} MBR_Partition;

typedef struct MBR_Data
{
    uint64_t count;
    MBR_Partition partitions[4];
} MBR_Data;

typedef struct MBR_Raw_Partition
{
    uint8_t bootFlag;
    uint8_t chsFirst[3];
    uint8_t type;
    uint8_t chsLast[3];
    uint32_t startLba;
    uint32_t sectorCount;
} __attribute__((packed)) MBR_Raw_Partition;

typedef struct MBR_Raw
{
    uint8_t code[446];
    MBR_Raw_Partition partitions[4];
    uint8_t signature[2];
} __attribute__((packed)) MBR_Raw;

static bool MBR_ReadFirstSector(Block_Device* device, MBR_Raw* raw)
{
    if (device->sectorSize == 512)
    {
        uint64_t read = device->read(device, 0, 1, (uint8_t*)raw);
        if (read != 1) return false;
    }
    else if (device->sectorSize < 512)
    {
        uint8_t tmp[1024]; // Will be enough
        const uint64_t sectorsToRead = (512 + device->sectorSize - 1) / device->sectorSize;

        uint64_t read = device->read(device, 0, sectorsToRead, tmp);
        if (read != sectorsToRead) return false;

        memcpy((uint8_t*)raw, tmp, 512);
    }
    else if (device->sectorSize <= 4096)
    {
        uint8_t tmp[4096];

        uint64_t read = device->read(device, 0, 1, tmp);
        if (read != 1) return false;

        memcpy((uint8_t*)raw, tmp, 512);
    }
    else // > 4096
    {
        uint8_t* tmp = Memory_KernelAllocate(device->sectorSize);
        if (!tmp) return false;

        uint64_t read = device->read(device, 0, 1, tmp);
        if (read != 1)
        {
            Memory_KernelFree(tmp);
            return false;
        }

        memcpy((uint8_t*)raw, tmp, 512);

        Memory_KernelFree(tmp);
    }

    return true;
}

bool MBR_CheckDisk(Block_Device* device)
{
    MBR_Raw mbr;
    if (!MBR_ReadFirstSector(device, &mbr))
        return false;

    if (mbr.signature[0] != 0x55 || mbr.signature[1] != 0xAA)
        return false;

    bool atLeastOneValidEntry = false;
    for (uint8_t i = 0; i < 4; i++)
    {
        MBR_Raw_Partition* partition = &mbr.partitions[i];

        if (partition->bootFlag != 0x00 && partition->bootFlag != 0x80)
            return false;

        if (partition->type != 0x00)
            atLeastOneValidEntry = true;

        if (partition->type == 0xEE)
            return false; // GPT
    }

    if (!atLeastOneValidEntry) return false;

    return true;
}

static uint64_t MBR_GetPartitionCount(Device_PartitionTable* table)
{
    MBR_Data* data = (MBR_Data*)table->data;
    return data->count;
}

static bool MBR_GetPartitionEntry(Device_PartitionTable* table, uint64_t index, uint64_t* outStart, uint64_t* outCount)
{
    MBR_Data* data = (MBR_Data*)table->data;

    if (index >= data->count) return false;

    MBR_Partition* part = &data->partitions[index];

    *outStart = (uint64_t)part->startLba;
    *outCount = (uint64_t)part->sectorCount;

    return true;
}

static void MBR_Destroy(Device_PartitionTable* table)
{
    Memory_KernelFree(table->data);
}

bool MBR_GetPartitionTable(Block_Device* device, Device_PartitionTable* table)
{
    MBR_Raw mbr;
    if (!MBR_ReadFirstSector(device, &mbr))
        return false;

    MBR_Data* data = (MBR_Data*)Memory_KernelAllocate(sizeof(MBR_Data));
    if (!data) return false;

    table->parent = device;
    
    table->getPartitionCount = MBR_GetPartitionCount;
    table->getPartitionEntry = MBR_GetPartitionEntry;
    table->destroy = MBR_Destroy;

    data->count = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        MBR_Raw_Partition* raw = &mbr.partitions[i];
        if (raw->type == 0)
            continue; // not set

        MBR_Partition* part = &data->partitions[data->count++];

        part->startLba = raw->startLba;
        part->sectorCount = raw->sectorCount;
        part->type = raw->type;
        part->bootable = raw->bootFlag == 0x80 ? true : false;
        part->set = true;
    }

    table->data = data;

    return true;
}
