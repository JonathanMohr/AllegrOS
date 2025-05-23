#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t id;
} Disk;

bool disk_Initialize(Disk *disk, uint8_t device);
bool disk_ReadSectors(Disk    *device,
                      uint32_t lba,
                      uint8_t  sector_count,
                      void    *buffer);