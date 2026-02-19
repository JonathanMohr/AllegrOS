#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "disk.h"

typedef struct {
    Disk* disk;
    uint64_t offset;
    uint64_t size;
} Partition;

void Partition_GetFSPartition(Partition* partition, Disk* disk);

bool Partition_ReadSectors(Partition* partition, uint64_t lba, uint64_t count, void* buffer);
