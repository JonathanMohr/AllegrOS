#include "device.h"

#include "../memory/memory.h"

#include <memory.h>
#include <stdint.h>

typedef struct Partition_Data
{
    uint64_t start;
    Block_Device* parent;
} Partition_Data;

static uint64_t Partition_Read(Block_Device* dev, uint64_t sector, uint64_t count, void* buffer)
{
    Partition_Data* data = (Partition_Data*)dev->data;

    if (sector >= dev->sectorCount || count == 0)
        return 0;

    if (count > dev->sectorCount - sector)
        count = dev->sectorCount - sector;

    return data->parent->read(data->parent, sector + data->start, count, buffer);
}

static uint64_t Partition_Write(Block_Device* dev, uint64_t sector, uint64_t count, const void* buffer)
{
    Partition_Data* data = (Partition_Data*)dev->data;

    if (sector >= dev->sectorCount || count == 0)
        return 0;

    if (count > dev->sectorCount - sector)
        count = dev->sectorCount - sector;

    return data->parent->write(data->parent, sector + data->start, count, buffer);
}

static void Partition_Destroy(Block_Device* dev)
{
    Memory_KernelFree(dev->data);
}

static const char Partition_Type[] = "PARTITION";

bool Device_PartitionTable_CreateBlockDevice(Block_Device* parent, Block_Device* device, const char* name, uint64_t start, uint64_t count)
{
    Partition_Data* data = (Partition_Data*)Memory_KernelAllocate(sizeof(Partition_Data));
    if (!data) return false;

    const char* nameEnd = name;
    while (*nameEnd) nameEnd++;

    const uint32_t nameLength = ((uint32_t)(nameEnd - name) > (sizeof(device->name) - 1)) ? (sizeof(device->name) - 1) : (uint32_t)(nameEnd - name);

    memset(device->name, '\0', sizeof(device->name));
    memcpy(device->name, name, nameLength);

    device->sectorSize = parent->sectorSize;
    device->sectorCount = count;

    device->type = Partition_Type;

    device->read = Partition_Read;
    device->write = Partition_Write;
    
    device->destroy = Partition_Destroy;

    data->parent = parent;
    data->start = start;

    device->data = data;

    return true;
}
