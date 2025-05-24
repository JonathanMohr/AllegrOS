#include "mbr.h"
#include "stdio.h"
#include <core/memory/memory.h>

typedef struct {
    // 0x00	1	Drive attributes (bit 7 set = active or bootable)
    uint8_t attributes;

    // 0x01	3	CHS Address of partition start
    uint8_t chsStart[3];

    // 0x04	1	Partition type
    uint8_t partitionType;

    // 0x05	3	CHS address of last partition sector
    uint8_t chsEnd[3];

    // 0x08	4	LBA of partition start
    uint32_t lbaStart;

    // 0x0C	4	Number of sectors in partition
    uint32_t size;

} __attribute__((packed)) MBR_Entry;

void MBR_DetectPartition(Partition* part, Disk* disk, void* partition)
{
    part->disk = disk;
    if (disk->id < 0x80)
    {
        //TODO
    }
    else
    {
        MBR_Entry* entry = (MBR_Entry*)segoffset_to_linear(partition);
        part->partitionOffset = entry->lbaStart;
        part->partitionSize = entry->size;
    }
}

#include "../../debug.h"

bool Partition_ReadSectors(Partition* part, uint32_t lba, uint8_t sectors, void* lowerDataOut)
{
    log_debug("P", "lba: 0x%x, offset: 0x%x, right: 0x%x", lba, part->partitionOffset, lba + part->partitionOffset);
    return disk_ReadSectors(part->disk, lba + part->partitionOffset, sectors, lowerDataOut);
}