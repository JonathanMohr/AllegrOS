#include "fat.h"
#include <stdint.h>

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


bool FAT_CheckDevice(const Block_Device* device)
{

}
