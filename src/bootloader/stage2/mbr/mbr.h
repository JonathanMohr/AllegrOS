#pragma once

#include <stdint.h>

typedef struct MBR_Partition {
    uint8_t status;
    uint8_t chs_first[3];
    uint8_t type;
    uint8_t chs_last[3];

    uint32_t lba_first;
    uint32_t lba_size;

} __attribute__((packed)) MBR_Partition;

typedef struct MBR_Bootsector {
    uint8_t bootcode[446];
    MBR_Partition partitions[4];
    uint16_t signature;

} __attribute__((packed)) MBR_Bootsector;

uint32_t MBR_GetFSStart();
uint32_t MBR_GetFSSize();
