#include "mbr.h"

extern MBR_Bootsector mbr_bootsector;

uint32_t MBR_GetFSStart()
{
    // TODO: I don't like that it's hardcoded

    return mbr_bootsector.partitions[1].lba_first;
}

uint32_t MBR_GetFSSize()
{
    // TODO: I don't like that it's hardcoded

    return mbr_bootsector.partitions[1].lba_size;
}
