#include "disk.h"
#include "io.h"

#include "../../debug.h"

#include "ata.h"
#include "fdc.h"

bool disk_Initialize(Disk *disk, uint8_t device, uint16_t* buffer)
{
    if (device >= 0x80)
    {
        uint8_t result = ATA_Initialize(disk, device, buffer);
        switch (result)
        {
            case ATA_SUCCESSFUL:
                return true;
            case ATA_NO_DRIVE:
            case ATA_DRIVE_ERROR:
            default:
                return false;
        }
    }
    else /*if (device < 0x80)*/
    {
        uint8_t result = FDC_Initialize(disk, device, buffer);
        switch (result)
        {
            case FDC_SUCCESSFUL:
                return true;
            default:
                return false;
        }
    }
}

bool disk_ReadSectors(Disk     *disk,
                      uint64_t  lba,
                      uint8_t   sector_count,
                      void     *buffer)
{
    if (disk->id >= 0x80)
    {
        uint8_t result = ATA_Read(disk, lba, sector_count, buffer);
        switch (result)
        {
            case ATA_SUCCESSFUL:
                return true;
            case ATA_SECTOR_COUNT:
            case ATA_TOO_HIGH_LBA:
            case ATA_28_BIT_TOO_HIGH_LBA:
            default:
                return false;
        }
    }
    else /*if (device < 0x80)*/
    {
        uint8_t result = FDC_Read(disk, lba, sector_count, buffer);
        switch (result)
        {
            case FDC_SUCCESSFUL:
                return true;
            default:
                return false;
        }
    }
}