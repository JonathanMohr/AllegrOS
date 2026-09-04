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

typedef struct FAT_Driver_Data
{
    FAT_BootSector bootsector;

    uint8_t* fatSectorBuffer;
    uint8_t* sectorBuffer;

    uint32_t freeClusterCount;
    uint32_t nextFreeCluster;

    uint32_t fatSector;
    uint32_t fatSize;

    uint32_t dataSector; // First sector of data area
    uint32_t dataSize;   // Size of data area in sectors

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

    char oemIdentifier[9];
} FAT_Driver_Data;


typedef struct FAT_Inode_Extra
{
    uint32_t startCluster;
} FAT_Inode_Extra;


typedef struct FAT_File_Extra
{
    uint32_t currentCluster;
} FAT_File_Extra;


#define FAT_INODE_NUMBER_ROOT 0xFFFFFFFFFFFFFFFF


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
        if (!device->read(device, blockIndex, 1, data->fatSectorBuffer))
            return FAT_SECTOR_ERROR;
        memcpy(buffer.buffer, data->fatSectorBuffer + offsetInBlock, firstPart);
        if (!device->read(device, blockIndex + 1, 1, data->fatSectorBuffer))
            return FAT_SECTOR_ERROR;
        memcpy(buffer.buffer + firstPart, data->fatSectorBuffer, secondPart);
    }
    else
    {
        if (!device->read(device, blockIndex, 1, data->fatSectorBuffer))
            return FAT_SECTOR_ERROR;
        memcpy(buffer.buffer, data->fatSectorBuffer + offsetInBlock, entrySize);
    }

    uint32_t rawValue;
    if (data->fatVersion == FAT_VERSION_12)
        rawValue = (cluster & 1) ? (buffer.e16 >> 4) : (buffer.e16 & 0x0FFF);
    else if (data->fatVersion == FAT_VERSION_16)
        rawValue = buffer.e16;
    else
        rawValue = buffer.e32;

    return rawValue;
}


static bool FAT_GetRoot(Filesystem_Driver* driver, Filesystem_Inode* out)
{
    FAT_Driver_Data* data = driver->data;
    FAT_Inode_Extra* extra = Memory_KernelAllocate(sizeof(FAT_Inode_Extra));
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
            size += (uint64_t)data->bytesPerSector * data->sectorsPerCluster;
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

    out->number = FAT_INODE_NUMBER_ROOT;
    out->referenceCount = 1;

    out->extra = extra;

    out->attributes = 0;
    out->type = FILESYSTEM_ENTRY_DIRECTORY;

    return true;
}


static void FAT_Destroy(Filesystem_Driver* driver)
{
    FAT_Driver_Data* data = driver->data;

    Memory_KernelFree(data->fatSectorBuffer);

    if (data->sectorBuffer)
        Memory_KernelFree(data->sectorBuffer);

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

    if (bytesPerSector % parent->sectorSize == 0)
        data->sectorBuffer = NULL;
    else if (parent->sectorSize % bytesPerSector == 0)
        data->sectorBuffer = Memory_KernelAllocate(parent->sectorSize);
    else if (parent->sectorSize < bytesPerSector)
        data->sectorBuffer = Memory_KernelAllocate((uint64_t)bytesPerSector + (uint64_t)parent->sectorSize);
    else
        data->sectorBuffer = Memory_KernelAllocate((uint64_t)parent->sectorSize * 2);

    if (bytesPerSector % parent->sectorSize != 0 && !data->sectorBuffer)
    {
        Memory_KernelFree(data);
        return false;
    }

    data->fatSectorBuffer = Memory_KernelAllocate(parent->sectorSize);
    if (!data->fatSectorBuffer)
    {
        if (data->sectorBuffer) Memory_KernelFree(data->sectorBuffer);
        Memory_KernelFree(data);
        return false;
    }

    data->freeClusterCount = 0; // TODO
    data->nextFreeCluster = 0; // TODO

    data->fatSector = (uint32_t)reservedSectors;
    data->fatSize = fatSize;
    
    data->dataSector = dataStartSector;
    data->dataSize = dataSectors;

    data->totalSectors = totalSectors;

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

    driver->data = data;

    return true;
}
