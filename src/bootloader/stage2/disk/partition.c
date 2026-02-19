#include "partition.h"

#include "mbr/mbr.h"

void Partition_GetFSPartition(Partition* partition, Disk* disk)
{
    partition->disk = disk;
    partition->offset = MBR_GetFSStart();
    partition->size = MBR_GetFSSize();
}

bool Partition_ReadSectors(Partition* partition, uint64_t lba, uint64_t count, void* buffer)
{
    if ((lba + count) > partition->size) return false;
    return Disk_ReadSectors(partition->disk, lba + partition->offset, count, buffer);
}
