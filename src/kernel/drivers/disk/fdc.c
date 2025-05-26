#include "fdc.h"

void FDC_LBA_to_CHS(uint64_t lba, uint8_t* head, uint8_t* track, uint8_t* sector) {
    const uint8_t SECTORS_PER_TRACK = 18;
    const uint8_t HEADS = 2;

    *track = lba / (HEADS * SECTORS_PER_TRACK);
    *head = (lba / SECTORS_PER_TRACK) % HEADS;
    *sector = (lba % SECTORS_PER_TRACK) + 1; // Sectors are 1-based
}

uint8_t FDC_Read(Disk *disk, uint64_t lba, uint8_t sector_count, void *buffer)
{
    // TODO
}

uint8_t FDC_Initialize(Disk *disk, uint8_t device, uint16_t* buffer)
{
    // TODO
}