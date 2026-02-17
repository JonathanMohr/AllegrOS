#pragma once

#include "disk.h"

#define FDC_SUCCESSFUL              0x0

uint8_t FDC_Read(Disk *disk, uint64_t lba, uint8_t sector_count, void *buffer);

uint8_t FDC_Initialize(Disk *disk, uint8_t device, uint16_t* buffer);