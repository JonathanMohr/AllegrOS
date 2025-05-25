#include "fat.h"
#include "../disk/mbr.h"
#include "../../memory/memory.h"
#include <stddef.h>
#include "fatdefs.h"
#include "../../debug.h"

typedef struct
{
    FAT_BootSector BootSector;

    FAT_FileData* RootDirectory;

    //FAT_FileData OpenedFiles[MAX_FILE_HANDLES];
    FAT_FileData* OpenedFiles;

    //uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint8_t* FatCache;
    uint32_t FatCachePosition;

    //FAT_LFNBlock LFNBlocks[FAT_LFN_LAST];
    FAT_LFNBlock* LFNBlocks;
    int LFNCount;

} FAT_Data;

static FAT_Data* g_Data;
static uint32_t g_DataSectionLba;
static uint8_t g_FatType;
static uint32_t g_TotalSectors;
static uint32_t g_SectorsPerFat;

int FAT_CompareLFNBlocks(const void* blockA, const void* blockB)
{
    FAT_LFNBlock* a = (FAT_LFNBlock*)blockA;
    FAT_LFNBlock* b = (FAT_LFNBlock*)blockB;
    return ((int)a->Order) - ((int)b->Order);
}

bool FAT_ReadBootSector(Partition* disk)
{
    return Partition_ReadSectors(disk, 0, 1, &g_Data->BootSector);
}

bool FAT_ReadFat(Partition* disk, size_t lbaIndex)
{
    return Partition_ReadSectors(disk, g_Data->BootSector.ReservedSectors + lbaIndex, FAT_CACHE_SIZE, g_Data->FatCache);
}

void FAT_Detect(Partition* disk)
{
    uint32_t dataClusters = (g_TotalSectors - g_DataSectionLba) / g_Data->BootSector.SectorsPerCluster;
    if (dataClusters < 0xFF5) 
        g_FatType = 12;
    else if (g_Data->BootSector.SectorsPerFat != 0)
        g_FatType = 16;
    else
        g_FatType = 32;
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data->BootSector.SectorsPerCluster;
}

bool FAT_Allocate()
{
    g_Data->RootDirectory = (FAT_FileData*)memory_Allocate(sizeof(FAT_FileData), 1);
    if (!g_Data->RootDirectory)
    {
        log_err("FAT", "Couldn't allocate memory for FAT root directory.");
        return false;
    }
    //TODO: remove MAX_FILE_HANDLES
    g_Data->OpenedFiles = (FAT_FileData*)memory_Allocate(sizeof(FAT_FileData) * MAX_FILE_HANDLES, 1);
    if (!g_Data->OpenedFiles)
    {
        log_err("FAT", "Couldn't allocate memory for FAT file handles.");
        return false;
    }
    //TODO: remove FAT_CACHE_SIZE * SECTOR_SIZE
    g_Data->FatCache = (uint8_t*)memory_Allocate(FAT_CACHE_SIZE * SECTOR_SIZE, 1);
    if (!g_Data->FatCache)
    {
        log_err("FAT", "Couldn't allocate memory for FAT cache.");
        return false;
    }
    //TODO: remove FAT_LFN_LAST
    g_Data->LFNBlocks = (FAT_LFNBlock*)memory_Allocate(sizeof(FAT_LFNBlock) * FAT_LFN_LAST, 1);
    if (!g_Data->LFNBlocks)
    {
        log_err("FAT", "Couldn't allocate memory for FAT long file name blocks.");
        return false;
    }

    return true;
}

bool FAT_Initialize(Partition* disk)
{
    g_Data = (FAT_Data*)memory_Allocate(sizeof(FAT_Data), 1);
    if (!g_Data)
    {
        log_err("FAT", "Couldn't allocate memory for FAT data.");
        return false;
    }

    if (!FAT_ReadBootSector(disk))
    {
        log_err("FAT", "Read boot sector failed.");
        return false;
    }

    if (!FAT_Allocate())
    {
        log_err("FAT", "Couldn't allocate enough memory.");
        return false;
    }

    g_Data->FatCachePosition = 0xFFFFFFFF;

    g_TotalSectors = g_Data->BootSector.TotalSectors;
    if (g_TotalSectors == 0) {          // fat32
        g_TotalSectors = g_Data->BootSector.LargeSectorCount;
    }

    bool isFat32 = false;
    g_SectorsPerFat = g_Data->BootSector.SectorsPerFat;
    if (g_SectorsPerFat == 0) {         // fat32
        isFat32 = true;
        g_SectorsPerFat = g_Data->BootSector.EBR32.SectorsPerFat;
    }

    uint32_t rootDirLba;
    uint32_t rootDirSize;
    if (isFat32) {
        g_DataSectionLba = g_Data->BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BootSector.FatCount;
        rootDirLba = FAT_ClusterToLba( g_Data->BootSector.EBR32.RootDirectoryCluster);
        rootDirSize = 0;
    }
    else {
        rootDirLba = g_Data->BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BootSector.FatCount;
        rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BootSector.DirEntryCount;
        int32_t rootDirSectors = (rootDirSize + g_Data->BootSector.BytesPerSector - 1) / g_Data->BootSector.BytesPerSector;
        g_DataSectionLba = rootDirLba + rootDirSectors;
    }

    g_Data->RootDirectory->Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory->Public.IsDirectory = true;
    g_Data->RootDirectory->Public.Position = 0;
    g_Data->RootDirectory->Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BootSector.DirEntryCount;
    g_Data->RootDirectory->Opened = true;
    g_Data->RootDirectory->FirstCluster = rootDirLba;
    g_Data->RootDirectory->CurrentCluster = rootDirLba;
    g_Data->RootDirectory->CurrentSectorInCluster = 0;

    if (!Partition_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory->Buffer))
    {
        log_err("FAT", "Read root directory failed.");
        return false;
    }

    // calculate data section
    FAT_Detect(disk);

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;
    g_Data->LFNCount = 0;

    return true;
}