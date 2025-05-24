#include "fat.h"
#include "../disk/mbr.h"
#include "../../memory/memory.h"
#include <stddef.h>

static uint8_t* BootSector = NULL;

bool FAT_ReadBootSector(Partition* disk)
{
    if (!BootSector)
        return false;
    return Partition_ReadSectors(disk, 0, 1, BootSector);
}

bool FAT_Initialize(Partition* disk)
{
    BootSector = memory_Allocate(512, 1);
    //TODO: not working
    if (!FAT_ReadBootSector(disk))
        return false;
}