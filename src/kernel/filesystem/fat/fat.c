#include "fat.h"

#include <stdint.h>
#include <minmax.h>
#include <memory.h>
#include "../../memory/memory.h"
#include "stddef.h"

#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY144     ((uint8_t)0xF0) // 1.44 MB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY120     ((uint8_t)0xF4) // 1.2 MB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY720     ((uint8_t)0xF9) // 720 KB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY400     ((uint8_t)0xFD) // 400 KB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY360_OLD ((uint8_t)0xFF) // 360 KB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY360     ((uint8_t)0xF6) // 360 KB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_FLOPPY320     ((uint8_t)0xF7) // 320 KB
#define FAT_BOOTSECTOR_MEDIA_DESCRIPTOR_DISK          ((uint8_t)0xF8)

#define FAT_BOOTSECTOR_NO_EXTENDED_BOOT_SIGNATURE     ((uint8_t)0x00)
#define FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE_OLD    ((uint8_t)0x28)
#define FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE        ((uint8_t)0x29)

typedef struct FAT12_16_EBPB
{
    uint8_t driveNumber;

    uint8_t reserved;
    uint8_t bootSignature;

    // FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE
    uint32_t volumeID;

    // FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE or FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE_OLD
    uint8_t volumeLabel[11];
    uint8_t filesystemType[8];
} __attribute__((packed)) FAT12_16_EBPB;

typedef struct FAT32_EBPB
{
    uint32_t fatSize32;
    uint16_t extFlags;
    uint16_t fsVersion;
    uint32_t rootCluster;
    uint16_t fsInfoSector;
    uint16_t backupBootSector;
    uint8_t reserved1[12];

    uint8_t driveNumber;

    uint8_t reserved2;
    uint8_t bootSignature;

    // FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE
    uint32_t volumeID;

    // FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE or FAT_BOOTSECTOR_EXTENDED_BOOT_SIGNATURE_OLD
    uint8_t volumeLabel[11];
    uint8_t filesystemType[8];
} __attribute__((packed)) FAT32_EBPB;

typedef struct FAT_BootSector_Header
{
    uint8_t oemIdentifier[8];

    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;

    uint8_t fatCount;
    uint16_t rootDirEntryCount;
    uint16_t totalSectors;

    uint8_t mediaDescriptor;

    uint16_t fatSize; // sectors

    uint16_t sectorsPerTrack;
    uint16_t numberOfHeads;

    uint32_t hiddenSectors;
    uint32_t largeTotalSectors;

    union
    {
        FAT12_16_EBPB fat12_16;
        FAT32_EBPB fat32;
    } ebpb;

} __attribute__((packed)) FAT_BootSector_Header;

typedef struct FAT_BootSector
{
    uint8_t jmp[3];
    FAT_BootSector_Header header;
    uint8_t bootcode[420];
    uint8_t signature[2];
} __attribute__((packed)) FAT_BootSector;

typedef struct FAT32_FS_Info
{
    uint32_t leadSignature;
    uint8_t reserved1[480];
    uint32_t structSignature;
    uint32_t freeClusterCount;
    uint32_t nextFreeCluster;
    uint8_t reserved2[12];
    uint32_t trailSignature;
} __attribute__((packed)) FAT32_FS_Info;


#define FAT_ENTRY_FREE          ((uint8_t)0x00)
#define FAT_ENTRY_DELETED       ((uint8_t)0xE5)
#define FAT_ENTRY_KANJI_ESCAPE  ((uint8_t)0x05)

#define FAT_ENTRY_LFN_ATTRIBUTE ((uint8_t)0x0F)

#define FAT_ENTRY_READ_ONLY     ((uint8_t)0x01)
#define FAT_ENTRY_HIDDEN        ((uint8_t)0x02)
#define FAT_ENTRY_SYSTEM        ((uint8_t)0x04)
#define FAT_ENTRY_VOLUME_LABEL  ((uint8_t)0x08)
#define FAT_ENTRY_DIRECTORY     ((uint8_t)0x10)
#define FAT_ENTRY_ARCHIVE       ((uint8_t)0x20)

typedef struct FAT_DirectoryEntry
{
    uint8_t name[8];
    uint8_t ext[3];

    uint8_t attribute;

    uint8_t reserved;

    uint8_t creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;

    uint16_t lastAccessDate;

    uint16_t firstClusterHigh; // high word, only FAT32
    
    uint16_t lastModificationTime;
    uint16_t lastModificationDate;

    uint16_t firstCluster; // low word

    uint32_t fileSize; // bytes
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct FAT_LFNEntry
{
    uint8_t order;

    uint16_t name1[5];

    uint8_t attr;
    uint8_t reserved1;
    uint8_t checksum;

    uint16_t name2[6];

    uint16_t reserved2;

    uint16_t name3[2];
} __attribute__((packed)) FAT_LFNEntry;


static bool FAT_ReadFirstSector(Block_Device* device, FAT_BootSector* bootsector)
{
    if (device->sectorSize == 512)
    {
        uint64_t read = device->read(device, 0, 1, (uint8_t*)bootsector);
        if (read != 1) return false;
    }
    else if (device->sectorSize < 512)
    {
        uint8_t tmp[1024]; // Will be enough
        const uint64_t sectorsToRead = (512 + device->sectorSize - 1) / device->sectorSize;

        uint64_t read = device->read(device, 0, sectorsToRead, tmp);
        if (read != sectorsToRead) return false;

        memcpy((uint8_t*)bootsector, tmp, 512);
    }
    else if (device->sectorSize <= 4096)
    {
        uint8_t tmp[4096];

        uint64_t read = device->read(device, 0, 1, tmp);
        if (read != 1) return false;

        memcpy((uint8_t*)bootsector, tmp, 512);
    }
    else // > 4096
    {
        uint8_t* tmp = Memory_KernelAllocate(device->sectorSize);
        if (!tmp) return false;

        uint64_t read = device->read(device, 0, 1, tmp);
        if (read != 1)
        {
            Memory_KernelFree(tmp);
            return false;
        }

        memcpy((uint8_t*)bootsector, tmp, 512);

        Memory_KernelFree(tmp);
    }

    return true;
}


typedef uint8_t FAT_Version;
#define FAT_VERSION_12  ((FAT_Version)0)
#define FAT_VERSION_16  ((FAT_Version)1)
#define FAT_VERSION_32  ((FAT_Version)2)

#define FAT_ACTIVE_ALL 0xFFFF

#define FAT_CLUSTER_BUFFERING_OPTIMAL_SMALL 0
#define FAT_CLUSTER_BUFFERING_OPTIMAL_BIG   1
#define FAT_CLUSTER_BUFFERING_GOOD_ENOUGH   2
#define FAT_CLUSTER_BUFFERING_BAD_SMALL     3
#define FAT_CLUSTER_BUFFERING_BAD_BIG       4

typedef struct FAT_Driver_Data
{
    FAT_BootSector bootsector;

    uint8_t* fatSectorBuffer;
    uint8_t* clusterReadBuffer;
    uint8_t* rootDirBuffer;

    void* clusterBuffer;

    uint32_t freeClusterCount;
    uint32_t nextFreeCluster;

    uint32_t fatSector;
    uint32_t fatSize;

    uint32_t dataSector; // First sector of data area
    uint32_t dataSize;   // Size of data area in sectors

    uint32_t bytesPerCluster;
    uint32_t totalSectors;

    union
    {
        uint32_t cluster; // FAT32
        struct
        {
            uint32_t sector;
            uint16_t entryCount; // FAT12 / FAT16
        } fixed;
    } rootDir;

    uint16_t bytesPerSector;
    uint16_t sectorsPerCluster;
    uint16_t reservedSectors;

    uint16_t activeFat;

    FAT_Version fatVersion;

    uint8_t fatCount;
    uint8_t mediaDescriptor;

    uint8_t clusterBuffering;

    char oemIdentifier[9];
} FAT_Driver_Data;


typedef struct FAT_Node_Extra
{
    uint32_t startCluster;
} FAT_Node_Extra;


typedef struct FAT_File_Extra
{
    uint32_t currentCluster;
} FAT_File_Extra;


#define FAT_NODE_NUMBER_ROOT 0xFFFFFFFFFFFFFFFF


#define FAT_SECTOR_NORMAL 0
#define FAT_SECTOR_FREE   1
#define FAT_SECTOR_BAD    2
#define FAT_SECTOR_EOC    3
#define FAT_SECTOR_ERROR  0xFFFFFFFF
static inline uint32_t FAT_Cluster(FAT_Version version, uint32_t cluster)
{
    if (cluster == FAT_SECTOR_ERROR)
        return FAT_SECTOR_ERROR;

    switch (version)
    {
        case FAT_VERSION_12:
        {
            if (cluster == 0x000)
                return FAT_SECTOR_FREE;
            if (cluster == 0x001)
                return FAT_SECTOR_ERROR;
            if (cluster <  0xFF7)
                return FAT_SECTOR_NORMAL;
            if (cluster == 0xFF7)
                return FAT_SECTOR_BAD;
            if (cluster <= 0xFFF)
                return FAT_SECTOR_EOC;
            return FAT_SECTOR_ERROR;
        }

        case FAT_VERSION_16:
        {
            if (cluster == 0x0000)
                return FAT_SECTOR_FREE;
            if (cluster == 0x0001)
                return FAT_SECTOR_ERROR;
            if (cluster <  0xFFF7)
                return FAT_SECTOR_NORMAL;
            if (cluster == 0xFFF7)
                return FAT_SECTOR_BAD;
            if (cluster <= 0xFFFF)
                return FAT_SECTOR_EOC;
            return FAT_SECTOR_ERROR;
        }

        case FAT_VERSION_32:
        {
            if (cluster == 0x00000000)
                return FAT_SECTOR_FREE;
            if (cluster == 0x00000001)
                return FAT_SECTOR_ERROR;
            if (cluster <  0x0FFFFFF7)
                return FAT_SECTOR_NORMAL;
            if (cluster == 0x0FFFFFF7)
                return FAT_SECTOR_BAD;
            if (cluster <= 0x0FFFFFFF)
                return FAT_SECTOR_EOC;
            return FAT_SECTOR_ERROR;
        }

        default:
            return FAT_SECTOR_ERROR;
    }
}


static uint32_t FAT_ReadFAT(FAT_Driver_Data* data, Block_Device* device, uint32_t cluster)
{
    uint64_t fatIndex;
    uint8_t entrySize;
    if (data->fatVersion == FAT_VERSION_12)
    {
        fatIndex = cluster * 3 / 2;
        entrySize = 2;
    }
    else if (data->fatVersion == FAT_VERSION_16)
    {
        fatIndex = cluster * 2;
        entrySize = 2;
    }
    else
    {
        fatIndex = cluster * 4;
        entrySize = 4;
    }

    uint64_t fatStartSector = data->fatSector;
    if (data->activeFat != FAT_ACTIVE_ALL)
        fatStartSector += (uint64_t)data->fatSize * (uint64_t)data->activeFat;

    const uint64_t blockSize = device->sectorSize;
    uint64_t fatStartByte = (uint64_t)fatStartSector * data->bytesPerSector;
    uint64_t absoluteByteOffset = fatStartByte + fatIndex;

    uint64_t blockIndex = absoluteByteOffset / blockSize;
    uint64_t offsetInBlock = absoluteByteOffset % blockSize;

    union
    {
        uint8_t buffer[4];
        uint16_t e16;
        uint32_t e32;
    } buffer;

    if ((offsetInBlock + entrySize) > blockSize)
    {
        const uint64_t firstPart = blockSize - offsetInBlock;
        const uint64_t secondPart = entrySize - firstPart;
        if (device->read(device, blockIndex, 2, data->fatSectorBuffer) != 2)
            return FAT_SECTOR_ERROR;
        memcpy(buffer.buffer, data->fatSectorBuffer + offsetInBlock, firstPart);
        memcpy(buffer.buffer + firstPart, data->fatSectorBuffer + blockSize, secondPart);
    }
    else
    {
        if (device->read(device, blockIndex, 1, data->fatSectorBuffer) != 1)
            return FAT_SECTOR_ERROR;
        memcpy(buffer.buffer, data->fatSectorBuffer + offsetInBlock, entrySize);
    }

    uint32_t rawValue;
    if (data->fatVersion == FAT_VERSION_12)
        rawValue = (cluster % 2 == 1) ? (buffer.e16 >> 4) : (buffer.e16 & 0x0FFF);
    else if (data->fatVersion == FAT_VERSION_16)
        rawValue = buffer.e16;
    else
        rawValue = buffer.e32;

    return rawValue;
}

static bool FAT_WriteFAT(FAT_Driver_Data* data, Block_Device* device, uint32_t cluster, uint32_t value)
{
    // TODO: Active FAT
    
    uint64_t fatIndex;
    uint8_t entrySize;
    if (data->fatVersion == FAT_VERSION_12)
    {
        fatIndex = cluster * 3 / 2;
        entrySize = 2;
    }
    else if (data->fatVersion == FAT_VERSION_16)
    {
        fatIndex = cluster * 2;
        entrySize = 2;
    }
    else
    {
        fatIndex = cluster * 4;
        entrySize = 4;
    }

    uint64_t fatStartSector = data->fatSector;
    if (data->activeFat != FAT_ACTIVE_ALL)
        fatStartSector += (uint64_t)data->fatSize * (uint64_t)data->activeFat;

    const uint64_t blockSize = device->sectorSize;
    uint64_t fatStartByte = (uint64_t)fatStartSector * data->bytesPerSector;
    uint64_t absoluteByteOffset = fatStartByte + fatIndex;

    uint64_t blockIndex = absoluteByteOffset / blockSize;
    uint64_t offsetInBlock = absoluteByteOffset % blockSize;

    union
    {
        uint8_t buffer[4];
        uint16_t e16;
        uint32_t e32;
    } buffer;

    if (data->fatVersion == FAT_VERSION_12)
        buffer.e16 = (cluster % 2 == 1) ? (value << 4) : (value & 0x0FFF);
    else if (data->fatVersion == FAT_VERSION_16)
        buffer.e16 = (uint16_t)value;
    else
        buffer.e32 = value;

    if ((offsetInBlock + entrySize) > blockSize)
    {
        const uint64_t firstPart = blockSize - offsetInBlock;
        const uint64_t secondPart = entrySize - firstPart;
        if (device->read(device, blockIndex, 2, data->fatSectorBuffer) != 2)
            return false;
        if (data->fatVersion == FAT_VERSION_12)
        {
            if (cluster % 2 == 1)
                buffer.buffer[0] = data->fatSectorBuffer[offsetInBlock] & 0x0F;
            else
                buffer.buffer[1] = data->fatSectorBuffer[offsetInBlock + 1] & 0xF0;
        }
        memcpy(data->fatSectorBuffer + offsetInBlock, buffer.buffer, firstPart);
        memcpy(data->fatSectorBuffer + blockSize, buffer.buffer + firstPart, secondPart);

        if (device->write(device, blockIndex, 2, data->fatSectorBuffer) != 2)
            return false;
    }
    else
    {
        if (device->read(device, blockIndex, 1, data->fatSectorBuffer) != 1)
            return false;
        if (data->fatVersion == FAT_VERSION_12)
        {
            if (cluster % 2 == 1)
                buffer.buffer[0] = data->fatSectorBuffer[offsetInBlock] & 0x0F;
            else
                buffer.buffer[1] = data->fatSectorBuffer[offsetInBlock + 1] & 0xF0;
        }
        memcpy(data->fatSectorBuffer + offsetInBlock, buffer.buffer, entrySize);

        if (device->write(device, blockIndex, 1, data->fatSectorBuffer) != 1)
            return false;
    }

    return true;
}


static bool FAT_ReadRootDirectoryEntries(FAT_Driver_Data* data, Block_Device* device, uint16_t index, uint16_t count, void* out)
{
    if (data->fatVersion != FAT_VERSION_12 && data->fatVersion != FAT_VERSION_16)
        return false;

    if ((uint32_t)index + (uint32_t)count > data->rootDir.fixed.entryCount)
        return false;

    const uint64_t blockSize = device->sectorSize;
    const uint64_t rootDirStartByte = (uint64_t)data->rootDir.fixed.sector * data->bytesPerSector;
    const uint64_t byteOffset = rootDirStartByte + (uint64_t)index * sizeof(FAT_DirectoryEntry);
    const uint64_t size = (uint64_t)count * sizeof(FAT_DirectoryEntry);

    const uint64_t startBlock = byteOffset / blockSize;
    const uint64_t offsetInStartBlock = byteOffset % blockSize;

    if (offsetInStartBlock == 0 && size % blockSize == 0)
    {
        const uint64_t blockCount = size / blockSize;
        return device->read(device, startBlock, blockCount, out) == blockCount;
    }

    uint64_t remaining = size;
    uint64_t currentBlock = startBlock;
    uint64_t offsetInBlock = offsetInStartBlock;
    uint8_t* dst = (uint8_t*)out;

    while (remaining > 0)
    {
        if (device->read(device, currentBlock, 1, data->rootDirBuffer) != 1)
            return false;

        uint64_t chunk = blockSize - offsetInBlock;
        if (chunk > remaining)
            chunk = remaining;

        memcpy(dst, data->rootDirBuffer + offsetInBlock, chunk);

        dst += chunk;
        remaining -= chunk;
        offsetInBlock = 0;
        currentBlock++;
    }

    return true;
}

static bool FAT_WriteRootDirectoryEntries(FAT_Driver_Data* data, Block_Device* device, uint16_t index, uint16_t count, const void* in)
{
    if (data->fatVersion != FAT_VERSION_12 && data->fatVersion != FAT_VERSION_16)
        return false;

    if ((uint32_t)index + (uint32_t)count > data->rootDir.fixed.entryCount)
        return false;

    const uint64_t blockSize = device->sectorSize;
    const uint64_t rootDirStartByte = (uint64_t)data->rootDir.fixed.sector * data->bytesPerSector;
    const uint64_t byteOffset = rootDirStartByte + (uint64_t)index * sizeof(FAT_DirectoryEntry);
    const uint64_t size = (uint64_t)count * sizeof(FAT_DirectoryEntry);

    const uint64_t startBlock = byteOffset / blockSize;
    const uint64_t offsetInStartBlock = byteOffset % blockSize;

    if (offsetInStartBlock == 0 && size % blockSize == 0)
    {
        const uint64_t blockCount = size / blockSize;
        return device->write(device, startBlock, blockCount, in) == blockCount;
    }

    uint64_t remaining = size;
    uint64_t currentBlock = startBlock;
    uint64_t offsetInBlock = offsetInStartBlock;
    const uint8_t* src = (const uint8_t*)in;

    while (remaining > 0)
    {
        uint64_t chunk = blockSize - offsetInBlock;
        if (chunk > remaining)
            chunk = remaining;

        if (chunk < blockSize)
        {
            if (device->read(device, currentBlock, 1, data->rootDirBuffer) != 1)
                return false;
        }

        memcpy(data->rootDirBuffer + offsetInBlock, src, chunk);

        if (device->write(device, currentBlock, 1, data->rootDirBuffer) != 1)
            return false;

        src += chunk;
        remaining -= chunk;
        offsetInBlock = 0;
        currentBlock++;
    }

    return true;
}


static bool FAT_ReadCluster(FAT_Driver_Data* data, Block_Device* device, uint32_t cluster, void* out)
{
    if (cluster < 2)
        return false;

    const uint64_t blockSize = device->sectorSize;

    if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_OPTIMAL_SMALL)
    {
        const uint16_t blocksPerSector = (uint16_t)((uint64_t)data->bytesPerSector / blockSize);

        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartBlock = clusterStartSector * (uint64_t)blocksPerSector;

        const uint32_t blocksPerCluster = (uint32_t)((uint64_t)data->bytesPerCluster / blockSize);

        if (device->read(device, clusterStartBlock, blocksPerCluster, out) != blocksPerCluster)
            return false;
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_OPTIMAL_BIG)
    {
        const uint64_t sectorsPerBlock = blockSize / (uint64_t)data->bytesPerSector;

        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartBlock = clusterStartSector / (uint64_t)sectorsPerBlock;
        const uint64_t blocksPerCluster = (uint64_t)data->bytesPerCluster / blockSize;

        if (device->read(device, clusterStartBlock, blocksPerCluster, out) != blocksPerCluster)
            return false;
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_GOOD_ENOUGH)
    {
        const uint64_t sectorsPerBlock = blockSize / (uint64_t)data->bytesPerSector;

        const uint64_t clusterStartSector = ((uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster);

        const uint64_t startBlock = clusterStartSector / sectorsPerBlock;
        const uint64_t sectorOffsetInBlock = clusterStartSector % sectorsPerBlock;
        const uint64_t offsetInStartBlock = sectorOffsetInBlock * (uint64_t)data->bytesPerSector;

        const uint64_t blockCount = (offsetInStartBlock + (uint64_t)data->bytesPerCluster + blockSize - 1) / blockSize;

        if (device->read(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;

        memcpy(out, data->clusterReadBuffer + offsetInStartBlock, data->bytesPerCluster);
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_BAD_SMALL || data->clusterBuffering == FAT_CLUSTER_BUFFERING_BAD_BIG)
    {
        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartByte = clusterStartSector * data->bytesPerSector;

        const uint64_t startBlock = clusterStartByte / blockSize;
        const uint64_t offsetInStartBlock = clusterStartByte % blockSize;

        const uint64_t blockCount = (offsetInStartBlock + data->bytesPerCluster + blockSize - 1) / blockSize;

        if (device->read(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;

        memcpy(out, data->clusterReadBuffer + offsetInStartBlock, data->bytesPerCluster);
    }
    else
        return false;

    return true;
}

static bool FAT_WriteCluster(FAT_Driver_Data* data, Block_Device* device, uint32_t cluster, const void* in)
{
    if (cluster < 2)
        return false;

    const uint64_t blockSize = device->sectorSize;

    if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_OPTIMAL_SMALL)
    {
        const uint16_t blocksPerSector = (uint16_t)((uint64_t)data->bytesPerSector / blockSize);

        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartBlock = clusterStartSector * (uint64_t)blocksPerSector;

        const uint32_t blocksPerCluster = (uint32_t)((uint64_t)data->bytesPerCluster / blockSize);

        if (device->write(device, clusterStartBlock, blocksPerCluster, in) != blocksPerCluster)
            return false;
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_OPTIMAL_BIG)
    {
        const uint64_t sectorsPerBlock = blockSize / (uint64_t)data->bytesPerSector;

        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartBlock = clusterStartSector / (uint64_t)sectorsPerBlock;
        const uint64_t blocksPerCluster = (uint64_t)data->bytesPerCluster / blockSize;

        if (device->write(device, clusterStartBlock, blocksPerCluster, in) != blocksPerCluster)
            return false;
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_GOOD_ENOUGH)
    {
        const uint64_t sectorsPerBlock = blockSize / (uint64_t)data->bytesPerSector;

        const uint64_t clusterStartSector = ((uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster);

        const uint64_t startBlock = clusterStartSector / sectorsPerBlock;
        const uint64_t sectorOffsetInBlock = clusterStartSector % sectorsPerBlock;
        const uint64_t offsetInStartBlock = sectorOffsetInBlock * (uint64_t)data->bytesPerSector;

        const uint64_t blockCount = (offsetInStartBlock + (uint64_t)data->bytesPerCluster + blockSize - 1) / blockSize;

        if (device->read(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;

        memcpy(data->clusterReadBuffer + offsetInStartBlock, in, data->bytesPerCluster);

        if (device->write(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;
    }
    else if (data->clusterBuffering == FAT_CLUSTER_BUFFERING_BAD_SMALL || data->clusterBuffering == FAT_CLUSTER_BUFFERING_BAD_BIG)
    {
        const uint64_t clusterStartSector = (uint64_t)data->dataSector + (uint64_t)(cluster - 2) * (uint64_t)data->sectorsPerCluster;
        const uint64_t clusterStartByte = clusterStartSector * data->bytesPerSector;

        const uint64_t startBlock = clusterStartByte / blockSize;
        const uint64_t offsetInStartBlock = clusterStartByte % blockSize;

        const uint64_t blockCount = (offsetInStartBlock + data->bytesPerCluster + blockSize - 1) / blockSize;

        if (device->read(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;

        memcpy(data->clusterReadBuffer + offsetInStartBlock, in, data->bytesPerCluster);

        if (device->write(device, startBlock, blockCount, data->clusterReadBuffer) != blockCount)
            return false;
    }
    else
        return false;

    return true;
}


static bool FAT_GetRoot(Filesystem_Driver* driver, Filesystem_Node* out)
{
    FAT_Driver_Data* data = driver->data;
    FAT_Node_Extra* extra = Memory_KernelAllocate(sizeof(FAT_Node_Extra));
    if (!extra)
        return false;
    
    if (data->fatVersion == FAT_VERSION_32)
    {
        extra->startCluster = data->rootDir.cluster;
        uint64_t size = 0;
        uint32_t status;

        uint32_t cluster = data->rootDir.cluster;
        while ((status = FAT_Cluster(data->fatVersion, cluster)) == FAT_SECTOR_NORMAL)
        {
            size += (uint64_t)data->bytesPerSector * (uint64_t)data->sectorsPerCluster;
            cluster = FAT_ReadFAT(data, driver->parent, cluster);
        }

        if (status != FAT_SECTOR_EOC)
        {
            Memory_KernelFree(extra);
            return false;
        }

        out->size = size;
    }
    else
    {
        extra->startCluster = 0;
        out->size = (uint64_t)data->rootDir.fixed.entryCount * sizeof(FAT_DirectoryEntry);
    }

    out->number = FAT_NODE_NUMBER_ROOT;
    out->referenceCount = 1;

    out->extra = extra;

    out->attributes = 0;
    out->type = FILESYSTEM_ENTRY_DIRECTORY;

    return true;
}


static bool FAT_GetNode(Filesystem_Driver* driver, uint64_t number, Filesystem_Node* out)
{
    FAT_Driver_Data* data = driver->data;

    if (number == FAT_NODE_NUMBER_ROOT)
        return FAT_GetRoot(driver, out);

    uint32_t cluster = (uint32_t)(number >> 32);
    uint32_t index32 = (uint32_t)(number & 0xFFFFFFFF);
    if (index32 > 0xFFFF)
        return false;

    uint16_t index = (uint16_t)index32;

    FAT_DirectoryEntry entry;

    if (cluster == 0)
    {
        if (!FAT_ReadRootDirectoryEntries(data, driver->parent, index, 1, &entry))
            return false;
    }
    else
    {
        if (!FAT_ReadCluster(data, driver->parent, cluster, data->clusterBuffer))
            return false;

        uint32_t offsetInCluster = (uint32_t)index * sizeof(FAT_DirectoryEntry);
        if (offsetInCluster + sizeof(FAT_DirectoryEntry) > data->bytesPerCluster)
            return false;

        memcpy(&entry, data->clusterBuffer + offsetInCluster, sizeof(FAT_DirectoryEntry));
    }

    FAT_Node_Extra* extra = Memory_KernelAllocate(sizeof(FAT_Node_Extra));
    if (!extra)
        return false;

    extra->startCluster = (uint32_t)entry.firstCluster;
    if (data->fatVersion == FAT_VERSION_32)
        extra->startCluster |= (uint32_t)entry.firstClusterHigh << 16;

    uint64_t size = 0;
    if (entry.attribute & FAT_ENTRY_DIRECTORY)
    {
        uint32_t status;

        uint32_t dirCluster = extra->startCluster;
        while ((status = FAT_Cluster(data->fatVersion, dirCluster)) == FAT_SECTOR_NORMAL)
        {
            size += (uint64_t)data->bytesPerSector * (uint64_t)data->sectorsPerCluster;
            dirCluster = FAT_ReadFAT(data, driver->parent, dirCluster);
        }

        if (status != FAT_SECTOR_EOC)
        {
            Memory_KernelFree(extra);
            return false;
        }
    }
    else
        size = (uint64_t)entry.fileSize;

    out->number = number;
    out->size = size;
    out->referenceCount = 1;

    out->extra = extra;

    out->attributes = 0;
    if (entry.attribute & FAT_ENTRY_READ_ONLY)
        out->attributes |= FILESYSTEM_ATTRIBUTE_READONLY;
    if (entry.attribute & FAT_ENTRY_HIDDEN)
        out->attributes |= FILESYSTEM_ATTRIBUTE_HIDDEN;
    if (entry.attribute & FAT_ENTRY_SYSTEM)
        out->attributes |= FILESYSTEM_ATTRIBUTE_SYSTEM;

    out->type = FILESYSTEM_ENTRY_FILE;
    if (entry.attribute & FAT_ENTRY_DIRECTORY)
        out->type = FILESYSTEM_ENTRY_DIRECTORY;

    return true;
}

static bool FAT_RemoveNode(Filesystem_Driver* driver, Filesystem_Node* node)
{
    FAT_Driver_Data* data = driver->data;
    FAT_Node_Extra* extra = node->extra;

    bool anyError = false;
    uint32_t status;
    uint32_t cluster = extra->startCluster;

    while ((status = FAT_Cluster(data->fatVersion, cluster)) == FAT_SECTOR_NORMAL)
    {
        uint32_t next = FAT_ReadFAT(data, driver->parent, cluster);
        if (!FAT_WriteFAT(data, driver->parent, cluster, 0))
            anyError = true;
        cluster = next;
    }

    Memory_KernelFree(extra);

    if (status != FAT_SECTOR_EOC || anyError)
        return false;

    return true;
}


static uint64_t FAT_GetEntryCount(Filesystem_Driver* driver, Filesystem_Node* dir)
{
    FAT_Driver_Data* data = driver->data;
    FAT_Node_Extra* extra = dir->extra;

    uint64_t entryCount = 0;

    uint32_t status;
    uint32_t cluster = extra->startCluster;

        if (dir->number == FAT_NODE_NUMBER_ROOT && data->fatVersion != FAT_VERSION_32 && cluster == 0) // FAT12/16 root directory
    {
        FAT_DirectoryEntry entry;

        for (uint16_t i = 0; i < data->rootDir.fixed.entryCount; i++)
        {
            if (!FAT_ReadRootDirectoryEntries(data, driver->parent, i, 1, &entry))
                return FILESYSTEM_ENTRY_COUNT_ERROR;

            if (entry.attribute == FAT_ENTRY_LFN_ATTRIBUTE || entry.name[0] == FAT_ENTRY_DELETED || entry.attribute & FAT_ENTRY_VOLUME_LABEL)
                continue;

            if (entry.name[0] == FAT_ENTRY_FREE)
                break;

            entryCount++;
        }

        return entryCount;
    }

    if (cluster == 0)
        return 0;

    while ((status = FAT_Cluster(data->fatVersion, cluster)) == FAT_SECTOR_NORMAL)
    {
        if (!FAT_ReadCluster(data, driver->parent, cluster, data->clusterBuffer))
            return FILESYSTEM_ENTRY_COUNT_ERROR;
        cluster = FAT_ReadFAT(data, driver->parent, cluster);

        FAT_DirectoryEntry* entries = data->clusterBuffer;

        bool eod = false;
        for (uint32_t i = 0; i < data->bytesPerCluster / sizeof(FAT_DirectoryEntry); i++)
        {
            FAT_DirectoryEntry* entry = &entries[i];

            if (entry->attribute == FAT_ENTRY_LFN_ATTRIBUTE || entry->name[0] == FAT_ENTRY_DELETED || entry->attribute & FAT_ENTRY_VOLUME_LABEL)
                continue;

            if (entry->name[0] == FAT_ENTRY_FREE)
            {
                status = FAT_SECTOR_EOC;
                eod = true;
                break;
            }

            entryCount++;
        }

        if (eod) break;
    }

    if (status != FAT_SECTOR_EOC)
        return FILESYSTEM_ENTRY_COUNT_ERROR;

    return entryCount;
}

static uint8_t FAT_LFNChecksum(const uint8_t shortName[11])
{
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++)
        sum = (uint8_t)(((sum & 1) ? 0x80 : 0) + (sum >> 1) + shortName[i]);
    return sum;
}

static void FAT_ExtractLFNChars(const FAT_LFNEntry* lfn, uint16_t out13[13])
{
    memcpy(out13, lfn->name1, 5 * sizeof(uint16_t));
    memcpy(out13 + 5, lfn->name2, 6 * sizeof(uint16_t));
    memcpy(out13 + 11, lfn->name3, 2 * sizeof(uint16_t));
}

static uint32_t FAT_UTF16ToUTF8(const uint16_t* units, uint32_t count, char* out, uint32_t maxOut)
{
    uint32_t o = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        uint16_t c = units[i];
        if (c == 0x0000 || c == 0xFFFF)
            break;

        if (c < 0x80)
        {
            if (o + 1 > maxOut) break;
            out[o++] = (char)c;
        }
        else if (c < 0x800)
        {
            if (o + 2 > maxOut) break;
            out[o++] = (char)(0xC0 | (c >> 6));
            out[o++] = (char)(0x80 | (c & 0x3F));
        }
        else
        {
            if (o + 3 > maxOut) break;
            out[o++] = (char)(0xE0 | (c >> 12));
            out[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (c & 0x3F));
        }
    }
    return o;
}

static bool FAT_UTF8ToUTF16(const char* name, uint16_t* out, uint32_t maxOut, uint32_t* outCount)
{
    uint32_t count = 0;
    const unsigned char* p = (const unsigned char*)name;

    while (*p)
    {
        uint32_t codepoint;
        uint32_t extraBytes;

        if ((*p & 0x80) == 0x00)
        {
            codepoint = *p;
            extraBytes = 0;
        }
        else if ((*p & 0xE0) == 0xC0)
        {
            codepoint = *p & 0x1F;
            extraBytes = 1;
        }
        else if ((*p & 0xF0) == 0xE0)
        {
            codepoint = *p & 0x0F;
            extraBytes = 2;
        }
        else if ((*p & 0xF8) == 0xF0)
        {
            codepoint = *p & 0x07;
            extraBytes = 3;
        }
        else
            return false;

        p++;

        for (uint32_t i = 0; i < extraBytes; i++)
        {
            if ((*p & 0xC0) != 0x80)
                return false;
            codepoint = (codepoint << 6) | (*p & 0x3F);
            p++;
        }

        if (codepoint == 0)
            return false;

        if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
            return false;

        if (codepoint <= 0xFFFF)
        {
            if (count + 1 > maxOut)
                return false;
            out[count++] = (uint16_t)codepoint;
        }
        else
        {
            if (count + 2 > maxOut)
                return false;
            uint32_t v = codepoint - 0x10000;
            out[count++] = (uint16_t)(0xD800 + (v >> 10));
            out[count++] = (uint16_t)(0xDC00 + (v & 0x3FF));
        }
    }

    *outCount = count;
    return true;
}

static void FAT_BuildShortName(const uint8_t rawName[11], char out[13])
{
    uint8_t base[8];
    memcpy(base, rawName, 8);
    if (base[0] == FAT_ENTRY_KANJI_ESCAPE)
        base[0] = 0xE5;

    const uint8_t* ext = rawName + 8;

    int len = 0;
    for (int i = 0; i < 8 && base[i] != ' '; i++)
        out[len++] = (char)base[i];

    if (ext[0] != ' ')
    {
        out[len++] = '.';
        for (int i = 0; i < 3 && ext[i] != ' '; i++)
            out[len++] = (char)ext[i];
    }

    out[len] = '\0';
}

/* TODO: NOT GOOD */
static bool FAT_ResolveParentNodeNumber(Filesystem_Driver* driver, uint32_t parentCluster, uint64_t* out)
{
    FAT_Driver_Data* data = driver->data;

    bool parentIsRoot = (data->fatVersion == FAT_VERSION_32)
        ? (parentCluster == data->rootDir.cluster)
        : (parentCluster == 0);

    if (parentIsRoot)
    {
        *out = FAT_NODE_NUMBER_ROOT;
        return true;
    }

    uint32_t grandparentCluster;
    bool foundDotDot = false;
    {
        if (!FAT_ReadCluster(data, driver->parent, parentCluster, data->clusterBuffer))
            return false;

        uint32_t entriesPerCluster = data->bytesPerCluster / sizeof(FAT_DirectoryEntry);
        FAT_DirectoryEntry* entries = (FAT_DirectoryEntry*)data->clusterBuffer;

        for (uint32_t i = 0; i < entriesPerCluster; i++)
        {
            if (entries[i].name[0] == '.' && entries[i].name[1] == '.' && entries[i].name[2] == ' ')
            {
                uint32_t c = (uint32_t)entries[i].firstCluster;
                if (data->fatVersion == FAT_VERSION_32)
                    c |= (uint32_t)entries[i].firstClusterHigh << 16;
                grandparentCluster = c;
                foundDotDot = true;
                break;
            }
        }
    }

    if (!foundDotDot)
        return false;

    bool grandparentIsFixedRoot = (data->fatVersion != FAT_VERSION_32 && grandparentCluster == 0);

    if (grandparentIsFixedRoot)
    {
        for (uint16_t i = 0; i < data->rootDir.fixed.entryCount; i++)
        {
            FAT_DirectoryEntry e;
            if (!FAT_ReadRootDirectoryEntries(data, driver->parent, i, 1, &e))
                return false;

            if (e.name[0] == FAT_ENTRY_FREE)
                return false;
            if (e.name[0] == FAT_ENTRY_DELETED || e.attribute == FAT_ENTRY_LFN_ATTRIBUTE)
                continue;

            if ((uint32_t)e.firstCluster == parentCluster)
            {
                *out = ((uint64_t)0 << 32) | i;
                return true;
            }
        }
        return false;
    }

    uint32_t cluster = grandparentCluster;
    uint32_t status;
    while ((status = FAT_Cluster(data->fatVersion, cluster)) == FAT_SECTOR_NORMAL)
    {
        if (!FAT_ReadCluster(data, driver->parent, cluster, data->clusterBuffer))
            return false;

        uint32_t entriesPerCluster = data->bytesPerCluster / sizeof(FAT_DirectoryEntry);
        FAT_DirectoryEntry* entries = (FAT_DirectoryEntry*)data->clusterBuffer;

        for (uint32_t i = 0; i < entriesPerCluster; i++)
        {
            if (entries[i].name[0] == FAT_ENTRY_FREE)
                return false;
            if (entries[i].name[0] == FAT_ENTRY_DELETED || entries[i].attribute == FAT_ENTRY_LFN_ATTRIBUTE)
                continue;

            uint32_t c = (uint32_t)entries[i].firstCluster;
            if (data->fatVersion == FAT_VERSION_32)
                c |= (uint32_t)entries[i].firstClusterHigh << 16;

            if (c == parentCluster)
            {
                *out = ((uint64_t)cluster << 32) | i;
                return true;
            }
        }

        cluster = FAT_ReadFAT(data, driver->parent, cluster);
    }

    return false;
}

static bool FAT_NameEqualsCaseInsensitive(const char* a, const char* b)
{
    while (*a && *b)
    {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';

        if (ca != cb)
            return false;

        a++;
        b++;
    }

    return *a == *b;
}

static uint8_t FAT_ReadEntry(Filesystem_Driver* driver, Filesystem_File* dir, Filesystem_Entry* out)
{
    FAT_Driver_Data* data = driver->data;
    FAT_File_Extra* fileExtra = dir->extra;
    FAT_Node_Extra* nodeExtra = dir->node->extra;

    bool rootDirectory = (dir->node->number == FAT_NODE_NUMBER_ROOT &&
                          data->fatVersion != FAT_VERSION_32 &&
                          nodeExtra->startCluster == 0);

    uint16_t lfnChars[20 * 13];
    uint8_t lfnExpected = 0;
    uint8_t lfnChecksum = 0;
    bool haveLfn = false;

    while (1)
    {
        union
        {
            FAT_DirectoryEntry entry;
            FAT_LFNEntry lfn;
        } entry;
        uint32_t entryCluster;
        uint32_t entryIndex;

        if (rootDirectory)
        {
            if (dir->pos >= data->rootDir.fixed.entryCount)
                return FILESYSTEM_DIR_END;

            entryIndex = (uint32_t)dir->pos;
            entryCluster = 0;

            if (!FAT_ReadRootDirectoryEntries(data, driver->parent, (uint16_t)entryIndex, 1, &entry.entry))
                return FILESYSTEM_DIR_ERROR;
        }
        else
        {
            uint32_t entriesPerCluster = data->bytesPerCluster / sizeof(FAT_DirectoryEntry);
            uint32_t indexInCluster = (uint32_t)(dir->pos % entriesPerCluster);

            if (indexInCluster == 0 && dir->pos != 0)
            {
                uint32_t status = FAT_Cluster(data->fatVersion, fileExtra->currentCluster);
                if (status != FAT_SECTOR_NORMAL)
                    return FILESYSTEM_DIR_END;
                fileExtra->currentCluster = FAT_ReadFAT(data, driver->parent, fileExtra->currentCluster);
            }

            uint32_t status = FAT_Cluster(data->fatVersion, fileExtra->currentCluster);
            if (status != FAT_SECTOR_NORMAL)
                return FILESYSTEM_DIR_END;

            if (!FAT_ReadCluster(data, driver->parent, fileExtra->currentCluster, data->clusterBuffer))
                return FILESYSTEM_DIR_ERROR;

            entryCluster = fileExtra->currentCluster;
            entryIndex = indexInCluster;
            memcpy(&entry.entry, (uint8_t*)data->clusterBuffer + (uint64_t)indexInCluster * sizeof(FAT_DirectoryEntry), sizeof(FAT_DirectoryEntry));
        }

        dir->pos++;

        if (entry.entry.name[0] == FAT_ENTRY_FREE)
            return FILESYSTEM_DIR_END;

        if (entry.entry.name[0] == FAT_ENTRY_DELETED)
        {
            haveLfn = false;
            continue;
        }

        if (entry.entry.attribute == FAT_ENTRY_LFN_ATTRIBUTE)
        {
            FAT_LFNEntry* lfn = &entry.lfn;

            uint8_t sequence = lfn->order & 0x1F;
            bool isLast = (lfn->order & 0x40) != 0;

            if (sequence == 0 || sequence > 20)
            {
                haveLfn = false;
                continue;
            }

            if (isLast)
            {
                lfnExpected = sequence;
                lfnChecksum = lfn->checksum;
                haveLfn = true;
            }
            else if (!haveLfn || sequence != lfnExpected - 1 || lfn->checksum != lfnChecksum)
            {
                haveLfn = false;
                continue;
            }
            else
                lfnExpected = sequence;

            uint16_t chars[13];
            FAT_ExtractLFNChars(lfn, chars);
            memcpy(&lfnChars[(sequence - 1) * 13], chars, sizeof(chars));

            continue;
        }

        if (entry.entry.attribute & FAT_ENTRY_VOLUME_LABEL)
        {
            haveLfn = false;
            continue;
        }

        bool useLfn = haveLfn && lfnExpected == 1 && FAT_LFNChecksum(entry.entry.name) == lfnChecksum;

        if (useLfn)
        {
            uint32_t written = FAT_UTF16ToUTF8(lfnChars, 20 * 13, out->name, sizeof(out->name) - 1);
            out->name[written] = '\0';
        }
        else
        {
            char shortName[13];
            FAT_BuildShortName(entry.entry.name, shortName);
            uint32_t len = 0;
            while (shortName[len] != '\0' && len < sizeof(out->name) - 1)
            {
                out->name[len] = shortName[len];
                len++;
            }
            out->name[len] = '\0';
        }

        haveLfn = false;

        uint32_t firstCluster = (uint32_t)entry.entry.firstCluster;
        if (data->fatVersion == FAT_VERSION_32)
            firstCluster |= (uint32_t)entry.entry.firstClusterHigh << 16;

        if (entry.entry.name[0] == '.' && entry.entry.name[1] == ' ')
            out->node = dir->node->number;
        else if (entry.entry.name[0] == '.' && entry.entry.name[1] == '.')
        {
            uint64_t parentNode;
            if (!FAT_ResolveParentNodeNumber(driver, firstCluster, &parentNode))
                return FILESYSTEM_DIR_ERROR;
            out->node = parentNode;
        }
        else
            out->node = ((uint64_t)entryCluster << 32) | entryIndex;

        return FILESYSTEM_DIR_ENTRY_FOUND;
    }
}

static bool FAT_OpenFile(Filesystem_Driver* driver, Filesystem_Node* node, Filesystem_File* out);
static void FAT_CloseFile(struct Filesystem_Driver* driver, Filesystem_File* file);

static uint8_t FAT_LookupEntry(struct Filesystem_Driver* driver, Filesystem_Node* dir, const char* name, Filesystem_Entry* out)
{
    Filesystem_File dirFile;
    if (!FAT_OpenFile(driver, dir, &dirFile))
        return FILESYSTEM_DIR_ERROR;

    uint8_t result;

    while (1)
    {
        result = FAT_ReadEntry(driver, &dirFile, out);

        if (result != FILESYSTEM_DIR_ENTRY_FOUND)
            break;

        if (FAT_NameEqualsCaseInsensitive(out->name, name))
            break;
    }

    FAT_CloseFile(driver, &dirFile);

    return result;
}


static bool FAT_CreateNode(Filesystem_Driver* driver, Filesystem_Node* dir, Filesystem_Entry_Type type,
                            Filesystem_Entry_Attribute attributes, const char* name, Filesystem_Node* out)
{
    uint32_t utf16Count;
    uint16_t utf16Name[20 * 13];

    if (!FAT_UTF8ToUTF16(name, utf16Name, 255, &utf16Count))
        return false;
    utf16Name[utf16Count++] = 0; // Terminator

    uint8_t charIndex = 0;
    char firstChars[2] = {'#', '#'};

    while (*name && charIndex < 2)
    {
        if (*name >= 'a' && *name <= 'z')
            firstChars[charIndex++] = *name + ('A' - 'a');
        else if (*name >= 'A' && *name <= 'Z')
            firstChars[charIndex++] = *name;
        else if (*name >= '0' && *name <= '9')
            firstChars[charIndex++] = *name;
        else if (*name == '_' || *name == '$' || *name == '\'' || *name == '@' ||
                 *name == '!' || *name == '(' || *name == ')'  || *name == '{' ||
                 *name == '}' || *name == '^' || *name == '#'  || *name == '&')
            firstChars[charIndex++] = *name;
        else
            firstChars[charIndex++] = '_';
        name++;
    }
}

static bool FAT_Link(Filesystem_Driver* driver, Filesystem_Node* dir, const char* name, Filesystem_Node* target)
{
    // TODO
}

static uint64_t FAT_Unlink(Filesystem_Driver* driver, Filesystem_Node* dir, const char* name)
{
    // TODO
}


static uint64_t FAT_Read(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, uint8_t* buffer)
{
    // TODO
}

static uint64_t FAT_Write(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, const uint8_t* buffer)
{
    // TODO
}

static bool FAT_Seek(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t pos)
{
    // TODO
}


static bool FAT_OpenFile(Filesystem_Driver* driver, Filesystem_Node* node, Filesystem_File* out)
{
    FAT_Node_Extra* nodeExtra = node->extra;

    FAT_File_Extra* fileExtra = Memory_KernelAllocate(sizeof(FAT_File_Extra));
    if (!fileExtra)
        return false;

    fileExtra->currentCluster = nodeExtra->startCluster;

    out->node = node;
    out->pos = 0;
    out->extra = fileExtra;

    return true;
}

static void FAT_CloseFile(struct Filesystem_Driver* driver, Filesystem_File* file)
{
    Memory_KernelFree(file->extra);
}

static bool FAT_Move(Filesystem_Driver* driver, Filesystem_Node* srcDir, const char* oldName, Filesystem_Node* dstDir, const char* newName)
{
    // TODO
}



static void FAT_Destroy(Filesystem_Driver* driver)
{
    FAT_Driver_Data* data = driver->data;

    Memory_KernelFree(data->clusterBuffer);

    if (data->rootDirBuffer)
        Memory_KernelFree(data->rootDirBuffer);

    Memory_KernelFree(data->fatSectorBuffer);

    if (data->clusterReadBuffer)
        Memory_KernelFree(data->clusterReadBuffer);

    Memory_KernelFree(driver->data);
}


static bool isPowerOfTwo(uint16_t v) { return v && !(v & (v - 1)); }

bool FAT_CheckDevice(Block_Device* device)
{
    FAT_BootSector bootsector;
    if (!FAT_ReadFirstSector(device, &bootsector))
        return false;

    if (bootsector.signature[0] != 0x55 || bootsector.signature[1] != 0xAA)
        return false;

    const uint16_t bytesPerSector = bootsector.header.bytesPerSector;
    const uint16_t sectorsPerCluster = bootsector.header.sectorsPerCluster;
    const uint16_t reservedSectors = bootsector.header.reservedSectors;

    const uint16_t rootDirEntryCount = bootsector.header.rootDirEntryCount;

    const uint16_t fatCount = bootsector.header.fatCount;
    const uint32_t fatSize = bootsector.header.fatSize ? bootsector.header.fatSize :bootsector.header.ebpb.fat32.fatSize32;
    const uint32_t totalSectors = bootsector.header.totalSectors ? bootsector.header.totalSectors : bootsector.header.largeTotalSectors;

    if (bytesPerSector != 512 && bytesPerSector != 1024 &&
        bytesPerSector != 2048 && bytesPerSector != 4096)
        return false;

    if (!isPowerOfTwo(sectorsPerCluster) || sectorsPerCluster > 128)
        return false;

    if (fatSize == 0 ||
        totalSectors == 0 ||
        fatCount == 0 ||
        reservedSectors == 0)
        return false;

    // TODO: ...

    return true;
}

bool FAT_GetDriver(Block_Device* parent, Filesystem_Driver* driver)
{
    FAT_Driver_Data* data = (FAT_Driver_Data*)Memory_KernelAllocate(sizeof(FAT_Driver_Data));
    if (!data)
        return false;

    if (parent->sectorSize < 3) return false; // TODO: Fix this

    if (!FAT_ReadFirstSector(parent, &data->bootsector))
    {
        Memory_KernelFree(data);
        return false;
    }

    if (data->bootsector.signature[0] != 0x55 || data->bootsector.signature[1] != 0xAA)
    {
        Memory_KernelFree(data);
        return false;
    }

    const uint16_t bytesPerSector = data->bootsector.header.bytesPerSector;
    const uint16_t sectorsPerCluster = data->bootsector.header.sectorsPerCluster;
    const uint32_t bytesPerCluster = (uint32_t)bytesPerSector * (uint32_t)sectorsPerCluster;
    const uint16_t reservedSectors = data->bootsector.header.reservedSectors;

    const uint16_t rootDirEntryCount = data->bootsector.header.rootDirEntryCount;

    const uint16_t fatCount = data->bootsector.header.fatCount;
    const uint32_t fatSize = data->bootsector.header.fatSize ? data->bootsector.header.fatSize : data->bootsector.header.ebpb.fat32.fatSize32;
    const uint32_t totalSectors = data->bootsector.header.totalSectors ? data->bootsector.header.totalSectors : data->bootsector.header.largeTotalSectors;

    if (bytesPerSector != 512 && bytesPerSector != 1024 &&
        bytesPerSector != 2048 && bytesPerSector != 4096)
    {
        Memory_KernelFree(data);
        return false;
    }

    if (!isPowerOfTwo(sectorsPerCluster) || sectorsPerCluster > 128)
    {
        Memory_KernelFree(data);
        return false;
    }

    if (fatSize == 0 ||
        totalSectors == 0 ||
        fatCount == 0 ||
        reservedSectors == 0)
    {
        Memory_KernelFree(data);
        return false;
    }

    const uint64_t reserved = (uint64_t)reservedSectors + ((uint64_t)fatCount * fatSize);
    if (reserved > totalSectors)
    {
        Memory_KernelFree(data);
        return false;
    }

    const uint32_t rootDirStartSector = (uint32_t)reserved;
    const uint16_t rootDirSectors = (uint16_t)((((uint32_t)rootDirEntryCount * 32) + (uint32_t)(bytesPerSector - 1)) / (uint32_t)bytesPerSector);

    const uint64_t nonDataSectors = ((uint64_t)reserved + (uint64_t)rootDirSectors);
    if (nonDataSectors > (uint64_t)totalSectors)
    {
        Memory_KernelFree(data);
        return false;
    }
    const uint32_t dataSectors = totalSectors - (uint32_t)nonDataSectors;
    const uint32_t dataStartSector = (uint32_t)nonDataSectors;
    const uint32_t clusterCount = dataSectors / sectorsPerCluster;

    const uint32_t clusterSize = (uint32_t)bytesPerSector * sectorsPerCluster;
    if (clusterSize > 32 * 1024)
    {
        Memory_KernelFree(data);
        return false;
    }

    FAT_Version version;
    if (clusterCount < 4085)
        version = FAT_VERSION_12;
    else if (clusterCount < 65525)
        version = FAT_VERSION_16;
    else // clusterCount >= 65525
        version = FAT_VERSION_32;

    if (version != FAT_VERSION_32 && rootDirEntryCount == 0)
    {
        Memory_KernelFree(data);
        return false;
    }
    if (version == FAT_VERSION_32 && rootDirEntryCount > 0)
    {
        Memory_KernelFree(data);
        return false;
    }

    if (version == FAT_VERSION_32)
    {
        const FAT32_EBPB* ebpb = &data->bootsector.header.ebpb.fat32;

        /*
            Bits 4-6 and 8-15
        */
        if (ebpb->extFlags & (0xFF70))
        {
            Memory_KernelFree(data);
            return false;
        }

        if (ebpb->fsVersion != 0)
        {
            Memory_KernelFree(data);
            return false;
        }
    }

    data->fatVersion = version;

    const uint64_t blockSize = parent->sectorSize;
    if ((uint64_t)bytesPerSector % blockSize == 0)
    {
        data->clusterReadBuffer = NULL;
        data->clusterBuffering = FAT_CLUSTER_BUFFERING_OPTIMAL_SMALL;
    }
    else if (blockSize % (uint64_t)bytesPerSector == 0 &&
             (uint64_t)dataStartSector % (blockSize / (uint64_t)bytesPerSector) == 0 &&
             (uint64_t)sectorsPerCluster % (blockSize / (uint64_t)bytesPerSector) == 0)
    {
        data->clusterReadBuffer = NULL;
        data->clusterBuffering = FAT_CLUSTER_BUFFERING_OPTIMAL_BIG;
    }
    else if (blockSize % (uint64_t)bytesPerSector == 0)
    {
        const uint64_t maxOffset = blockSize - (uint64_t)bytesPerSector;
        const uint64_t bufferBlocks = ((uint64_t)bytesPerCluster + maxOffset + blockSize - 1) / blockSize;
        data->clusterReadBuffer = Memory_KernelAllocate(bufferBlocks * blockSize);
        data->clusterBuffering = FAT_CLUSTER_BUFFERING_GOOD_ENOUGH;
    }
    else if (parent->sectorSize < bytesPerSector)
    {
        const uint64_t bufferBlocks = (uint64_t)((uint64_t)bytesPerCluster + blockSize - 1) / blockSize + 1;
        data->clusterReadBuffer = Memory_KernelAllocate((uint64_t)bufferBlocks * blockSize);
        data->clusterBuffering = FAT_CLUSTER_BUFFERING_BAD_SMALL;
    }
    else
    {
        const uint64_t bufferBlocks = (uint64_t)((uint64_t)bytesPerCluster + blockSize - 1) / blockSize + 1;
        data->clusterReadBuffer = Memory_KernelAllocate((uint64_t)bufferBlocks * blockSize);
        data->clusterBuffering = FAT_CLUSTER_BUFFERING_BAD_BIG;
    }

    if (data->clusterBuffering != FAT_CLUSTER_BUFFERING_OPTIMAL_SMALL && data->clusterBuffering != FAT_CLUSTER_BUFFERING_OPTIMAL_BIG && !data->clusterReadBuffer)
    {
        Memory_KernelFree(data);
        return false;
    }

    data->fatSectorBuffer = Memory_KernelAllocate(parent->sectorSize * 2);
    if (!data->fatSectorBuffer)
    {
        if (data->clusterReadBuffer) Memory_KernelFree(data->clusterReadBuffer);
        Memory_KernelFree(data);
        return false;
    }

    if (data->fatVersion == FAT_VERSION_12 || data->fatVersion == FAT_VERSION_16)
    {
        data->rootDirBuffer = Memory_KernelAllocate(blockSize);
        if (!data->rootDirBuffer)
        {
            Memory_KernelFree(data->fatSectorBuffer);
            if (data->clusterReadBuffer) Memory_KernelFree(data->clusterReadBuffer);
            Memory_KernelFree(data);
            return false;
        }
    }

    data->clusterBuffer = Memory_KernelAllocate(bytesPerCluster);
    if (!data->clusterBuffer)
    {
        if (data->rootDirBuffer)
            Memory_KernelFree(data->rootDirBuffer);

        Memory_KernelFree(data->fatSectorBuffer);

        if (data->clusterReadBuffer)
            Memory_KernelFree(data->clusterReadBuffer);

        Memory_KernelFree(driver->data);
    }

    data->freeClusterCount = 0; // TODO
    data->nextFreeCluster = 0; // TODO

    data->fatSector = (uint32_t)reservedSectors;
    data->fatSize = fatSize;
    
    data->dataSector = dataStartSector;
    data->dataSize = dataSectors;

    data->totalSectors = totalSectors;

    data->bytesPerCluster = bytesPerCluster;

    if (version != FAT_VERSION_32)
    {
        data->rootDir.fixed.sector = rootDirStartSector;
        data->rootDir.fixed.entryCount = rootDirEntryCount;
    }
    else
    {
        data->rootDir.cluster = data->bootsector.header.ebpb.fat32.rootCluster;
    }

    data->bytesPerSector = bytesPerSector;
    data->sectorsPerCluster = sectorsPerCluster;
    data->reservedSectors = reservedSectors;

    if (version != FAT_VERSION_32)
    {
        data->activeFat = FAT_ACTIVE_ALL;
    }
    else
    {
        const uint16_t extFlags = data->bootsector.header.ebpb.fat32.extFlags;

        /* Bit 7 */
        if (extFlags & 0x0080)
        {
            /* Bits 0-3 */
            data->activeFat = extFlags & 0x000F;
        }
        else
            data->activeFat = FAT_ACTIVE_ALL;
    }

    data->fatCount = fatCount;
    data->mediaDescriptor = data->bootsector.header.mediaDescriptor;

    memcpy(data->oemIdentifier, data->bootsector.header.oemIdentifier, sizeof(data->bootsector.header.oemIdentifier));


    driver->parent = parent;

    driver->destroy = FAT_Destroy;

    driver->getRoot = FAT_GetRoot;

    driver->getNode = FAT_GetNode;
    driver->removeNode = FAT_RemoveNode;

    driver->getEntryCount = FAT_GetEntryCount;
    driver->readEntry = FAT_ReadEntry;
    driver->lookupEntry = FAT_LookupEntry;

    driver->createNode = FAT_CreateNode;
    driver->link = FAT_Link;
    driver->unlink = FAT_Unlink;

    driver->read = FAT_Read;
    driver->write = FAT_Write;
    driver->seek = FAT_Seek;

    driver->openFile = FAT_OpenFile;
    driver->closeFile = FAT_CloseFile;
    driver->move = FAT_Move;

    driver->data = data;

    driver->caseSensitive = false;

    return true;
}
