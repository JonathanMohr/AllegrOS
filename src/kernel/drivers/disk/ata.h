#pragma once

#include "disk.h"

#define ATA_SUCCESSFUL              0x0
#define ATA_NO_DRIVE                0x1
#define ATA_SECTOR_COUNT            0x2
#define ATA_TOO_HIGH_LBA            0x3
#define ATA_28_BIT_TOO_HIGH_LBA     0x4
#define ATA_DRIVE_ERROR             0x5

uint8_t ATA_Read(Disk *disk, uint64_t lba, uint8_t sector_count, void *buffer);

uint8_t ATA_Initialize(Disk *disk, uint8_t device, uint16_t* buffer);