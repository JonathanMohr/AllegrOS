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

static void FAT_Destroy(Filesystem_Driver* driver)
{
    FAT_Driver_Data* data = driver->data;

    if (data->sectorBuffer)
        Memory_KernelFree(data->sectorBuffer);

    Memory_KernelFree(driver->data);
}


bool FAT_CheckDevice(Block_Device* device)
{
    FAT_BootSector bootsector;
    if (!FAT_ReadFirstSector(device, &bootsector))
        return false;

    if (bootsector.signature[0] != 0x55 || bootsector.signature[1] != 0xAA)
        return false;

    // TODO: ...

    return true;
}

static bool isPowerOfTwo(uint16_t v) { return v && !(v & (v - 1)); }

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
    const uint32_t fatSize = data->bootsector.header.fatSize ? data->bootsector.header.fatSize :data->bootsector.header.ebpb.fat32.fatSize32;
    const uint32_t totalSectors = data->bootsector.header.totalSectors ? data->bootsector.header.totalSectors : data->bootsector.header.largeTotalSectors;

    if (bytesPerSector == 0 ||
        sectorsPerCluster == 0 ||
        fatSize == 0 ||
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
    const uint32_t rootDirSectors = (((uint32_t)rootDirEntryCount * 32) + (bytesPerSector - 1)) / bytesPerSector;

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

    FAT_Version version;
    if (clusterCount < 4085)
        version = FAT_VERSION_12;
    else if (clusterCount < 65525)
        version = FAT_VERSION_16;
    else // clusterCount >= 65525
        version = FAT_VERSION_32;

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

    if (clusterSize > 32 * 1024)
    {
        Memory_KernelFree(data);
        return false;
    }

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
