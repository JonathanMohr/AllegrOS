#pragma once
#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    DISK* disk;
    uint32_t partitionOffset;
    uint32_t partitionSize;
} Partition;

void MBR_DetectPartition(Partition* part, DISK* disk, void* partition);

bool Partition_ReadSectors(Partition* part, uint32_t lba, uint8_t sectors, void* lowerDataOut);