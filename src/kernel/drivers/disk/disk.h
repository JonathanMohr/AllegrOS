#pragma once

#include <stdbool.h>
#include <stdint.h>

enum Disk_General {
    DISK_NOT_PRESENT       = 0x8000,   // Bit 15 = 1 kein Gerät

    DISK_NON_ATA           = 0x4000,   // Bit 14 = 1 Nicht-ATA Gerät (ATAPI)

    DISK_REMOVABLE         = 0x1000,   // Bit 13 = 1 Wechselmedium

    DISK_DEVICE_TYPE_MASK  = 0x1F00,   // Bits 12-8 (Gerätetyp, 5 Bit)

    DISK_DMA_SUPPORTED     = 0x0080,   // Bit 7 = 1 DMA unterstützt
};

enum Disk_Features {
    FEATURE_DMA_SUPPORTED        = 1 << 8,   // Bit 8:     DMA unterstützt
    FEATURE_LBA_SUPPORTED        = 1 << 9,   // Bit 9:     LBA unterstützt
    FEATURE_IORDY_SUPPORTED      = 1 << 10,  // Bit 10:    IORDY unterstützt
    FEATURE_RESERVED_11          = 1 << 11,  // Bit 11:    reserviert (meist 0)
    FEATURE_ATAPI_DEVICE         = 1 << 15,  // Bit 15:    ATAPI Gerät (CD-ROM etc.)
    // Bit 0-7, 12-14 sind in diesem Wort normalerweise nicht genutzt oder reserviert
};

enum Disk_Features_Ext {
    FEATURE_EXT_SMART_SUPPORTED      = 1 << 0,   // Bit 0: SMART unterstützt
    FEATURE_EXT_SECURITY_SUPPORTED   = 1 << 1,   // Bit 1: Sicherheitsfunktionen unterstützt
    FEATURE_EXT_POWER_MANAGEMENT     = 1 << 3,   // Bit 3: Energiesparfunktionen unterstützt
    FEATURE_EXT_WCACHE_SUPPORTED     = 1 << 4,   // Bit 4: Write Cache unterstützt
    FEATURE_EXT_LOOK_AHEAD          = 1 << 5,   // Bit 5: Look-Ahead unterstützt
    FEATURE_EXT_48BIT_LBA           = 1 << 10,  // Bit 10: 48-Bit LBA unterstützt
    FEATURE_EXT_RW_MULTIPLE         = 1 << 11,  // Bit 11: Multiple-Block-Read/Write unterstützt
    FEATURE_EXT_QUEUE_DEPTH         = 1 << 12,  // Bit 12: Native Command Queuing unterstützt
    FEATURE_EXT_CFA_SUPPORTED       = 1 << 13,  // Bit 13: CFA unterstützt
    FEATURE_EXT_PIO_MULTI           = 1 << 14,  // Bit 14: Multiword DMA unterstützt
    FEATURE_EXT_MSN                 = 1 << 15,  // Bit 15: (Meist reserviert / Hersteller-spezifisch)
};

typedef struct {
    uint8_t id;

    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;

    uint16_t general;

    char serial_number[21];     // 20 letters + '\0'
    char firmware_revision[9];  // 8 letters + '\0'
    char model_number[41];      // 40 letters + '\0'

    uint16_t max_sectors_per_rw;
    uint16_t supported_features;
    uint32_t total_sectors_28bit;
    uint16_t supported_features_ext;

    uint64_t total_sectors_48bit;
} Disk;

bool disk_Initialize(Disk *disk, uint8_t device, uint16_t* buffer);
bool disk_ReadSectors(Disk    *device,
                      uint64_t lba,
                      uint8_t  sector_count,
                      void    *buffer);