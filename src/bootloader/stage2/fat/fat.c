#include "fat.h"

#include <stddef.h>
#include <minmax.h>
#include <memory.h>
#include "../x86/x86.h"

#include "../io/io.h"

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           512
#define MAX_FILE_HANDLES        10
#define FAT_CACHE_SIZE          5
#define ROOT_DIRECTORY_HANDLE   -1

typedef struct FAT_FileData {
    uint8_t buffer[SECTOR_SIZE];
    FAT_File public;
    bool opened;
    uint32_t firstCluster;
    uint32_t currentCluster;
    uint32_t currentSectorInCluster;

} FAT_FileData;

static struct FAT_Data {
    union {
        FAT_BootSector bootSector;
        uint8_t bytes[SECTOR_SIZE];
    } BS;

    FAT_FileData rootDirectory;

    FAT_FileData openedFiles[MAX_FILE_HANDLES];

    uint8_t fatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t fatCachePosition;

} FAT_Data;
static uint32_t dataSectionLba;
static uint8_t fatType;
static uint32_t totalSectors;
static uint32_t sectorsPerFat;

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return dataSectionLba + (cluster - 2) * FAT_Data.BS.bootSector.SectorsPerCluster;
}

bool FAT_ReadFAT(Partition* partition, uint32_t lbaIndex)
{
    return Partition_ReadSectors(partition, FAT_Data.BS.bootSector.ReservedSectors + lbaIndex, FAT_CACHE_SIZE, FAT_Data.fatCache);
}

bool FAT_Initialize(Partition* partition)
{
    // Read boot sector
    if (!Partition_ReadSectors(partition, 0, 1, FAT_Data.BS.bytes))
    {
        IO_PutString(dbgout, "FAT: Read boot sector failed!\n");
        return false;
    }

    // Read FAT
    FAT_Data.fatCachePosition = 0xFFFFFFFF;

    totalSectors = FAT_Data.BS.bootSector.TotalSectors;
    if (totalSectors == 0)
        totalSectors = FAT_Data.BS.bootSector.LargeSectorCount;

    bool isFat32 = false;
    sectorsPerFat = FAT_Data.BS.bootSector.SectorsPerFat;
    if (sectorsPerFat == 0)
    {
        isFat32 = true;
        sectorsPerFat = FAT_Data.BS.bootSector.EBR32.SectorsPerFat;
    }

    uint32_t rootDirLba;
    uint32_t rootDirSize;
    if (isFat32)
    {
        dataSectionLba = FAT_Data.BS.bootSector.ReservedSectors + sectorsPerFat * FAT_Data.BS.bootSector.FatCount;
        rootDirLba = FAT_ClusterToLba(FAT_Data.BS.bootSector.EBR32.RootDirectoryCluster);
        rootDirSize = 0;
    }
    else
    {
        rootDirLba = FAT_Data.BS.bootSector.ReservedSectors + sectorsPerFat * FAT_Data.BS.bootSector.FatCount;
        rootDirSize = sizeof(FAT_DirectoryEntry) * FAT_Data.BS.bootSector.DirEntryCount;
        uint32_t rootDirSectors = (rootDirSize + FAT_Data.BS.bootSector.BytesPerSector - 1) / FAT_Data.BS.bootSector.BytesPerSector;
        dataSectionLba = rootDirLba + rootDirSectors;
    }

    FAT_Data.rootDirectory.public.handle = ROOT_DIRECTORY_HANDLE;
    FAT_Data.rootDirectory.public.isDirectory = true;
    FAT_Data.rootDirectory.public.position = 0;
    FAT_Data.rootDirectory.public.size = rootDirSize;
    FAT_Data.rootDirectory.opened = true;
    FAT_Data.rootDirectory.firstCluster = rootDirLba;
    FAT_Data.rootDirectory.currentCluster = rootDirLba;
    FAT_Data.rootDirectory.currentSectorInCluster = 0;

    if (!Partition_ReadSectors(partition, rootDirLba, 1, FAT_Data.rootDirectory.buffer))
    {
        IO_PutString(dbgout, "FAT: Read root directory failed!\n");
        return false;
    }

    if (isFat32)
    {
        fatType = 32;
    }
    else
    {
        uint32_t dataClusters = (totalSectors - dataSectionLba) / FAT_Data.BS.bootSector.SectorsPerCluster;
        if (dataClusters < 0xFF5) 
            fatType = 12;
        else
            fatType = 16;
    }

    for (int32_t i = 0; i < MAX_FILE_HANDLES; i++)
        FAT_Data.openedFiles[i].opened = false;

    return true;
}

FAT_File* FAT_OpenEntry(Partition* partition, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int32_t handle = -1;
    for (int32_t i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!FAT_Data.openedFiles[i].opened)
            handle = i;
    }

    if (handle < 0)
    {
        IO_PutString(dbgout, "FAT: Out of file handles!\n");
        return NULL;
    }

    FAT_FileData* fd = &FAT_Data.openedFiles[handle];
    fd->public.handle = handle;
    fd->public.isDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->Size;
    fd->firstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    if (!Partition_ReadSectors(partition, FAT_ClusterToLba(fd->currentCluster), 1, fd->buffer))
    {
        IO_PrintFormat(dbgout, "FAT: open entry failed - read error: cluster=%udd lba=%udd\n", fd->currentCluster, FAT_ClusterToLba(fd->currentCluster));
        for (int i = 0; i < 11; i++)
            IO_PrintFormat(dbgout, "%c", entry->Name[i]);
        IO_PutChar(dbgout, '\n');
        return false;
    }

    fd->opened = true;
    return &fd->public;
}

uint32_t FAT_NextCluster(Partition* partition, uint32_t currentCluster)
{
    uint32_t fatIndex;
    if      (fatType == 12) fatIndex = currentCluster * 3 / 2;
    else if (fatType == 16) fatIndex = currentCluster * 2;
    else /* 32 */           fatIndex = currentCluster * 4;

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    uint32_t fatIndexSectorEnd = (fatIndex + 1) / SECTOR_SIZE;

    if (fatIndexSector < FAT_Data.fatCachePosition ||
        fatIndexSectorEnd >= FAT_Data.fatCachePosition + FAT_CACHE_SIZE)
    {
        if (!FAT_ReadFAT(partition, fatIndexSector))
        {
            IO_PutStringCritical("FAT: Couldn't read FAT!\n");
            return 0;
        }
        FAT_Data.fatCachePosition = fatIndexSector;
    }

    fatIndex -= (FAT_Data.fatCachePosition * SECTOR_SIZE);

    uint32_t nextCluster;
    if (fatType == 12)
    {
        if (currentCluster % 2 == 0)
            nextCluster = (*(uint16_t*)(FAT_Data.fatCache + fatIndex)) & 0x0FFF;
        else
            nextCluster = (*(uint16_t*)(FAT_Data.fatCache + fatIndex)) >> 4;

        if (nextCluster >= 0xFF8)
            nextCluster |= 0x0FFFF000;
    }
    else if (fatType == 16)
    {
        nextCluster = *(uint16_t*)(FAT_Data.fatCache + fatIndex);
        if (nextCluster >= 0xFFF8)
            nextCluster |= 0x0FFF0000;
    }
    else /* 32 */
        nextCluster = *(uint32_t*)(FAT_Data.fatCache + fatIndex);

    return nextCluster;
}

uint32_t FAT_Read(Partition* partition, FAT_File* file, uint32_t byteCount, void* buffer)
{
    FAT_FileData* fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                        ? &FAT_Data.rootDirectory : &FAT_Data.openedFiles[file->handle];

    uint8_t* dataOut = (uint8_t*)buffer;

    if (!fd->public.isDirectory || (fd->public.isDirectory && fd->public.size != 0))
        byteCount = min(byteCount, fd->public.size - fd->public.position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(dataOut, fd->buffer + fd->public.position % SECTOR_SIZE, take);
        dataOut += take;
        fd->public.position += take;
        byteCount -= take;

        if (leftInBuffer == take)
        {
            fd->currentSectorInCluster++;
            if (fd->currentSectorInCluster >= FAT_Data.BS.bootSector.SectorsPerCluster)
            {
                fd->currentSectorInCluster = 0;
                if (fatType == 32 || fd->public.handle != ROOT_DIRECTORY_HANDLE)
                    fd->currentCluster = FAT_NextCluster(partition, fd->currentCluster);
                else
                    fd->currentCluster++;
            }

            if (fd->currentCluster == 0)
            {
                IO_PutString(dbgout, "FAT: Couldn't get next cluster!\n");
                break;
            }

            if (fd->public.handle == ROOT_DIRECTORY_HANDLE)
            {
                if (!Partition_ReadSectors(partition, fd->currentCluster + fd->currentSectorInCluster, 1, fd->buffer))
                {
                    IO_PutString(dbgout, "FAT: Read error!\n");
                    break;
                }
            }
            else
            {
                if (fd->currentCluster >= 0x0FFFFFF8)
                {
                    // Mark end of file
                    fd->public.size = fd->public.position;
                    break;
                }
                
                if (!Partition_ReadSectors(partition, FAT_ClusterToLba(fd->currentCluster) + fd->currentSectorInCluster, 1, fd->buffer))
                {
                    IO_PutString(dbgout, "FAT: Read error!\n");
                    break;
                }
            }
        }
    }

    return dataOut - (uint8_t*)buffer;
}

bool FAT_ReadEntry(Partition* partition, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(partition, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File* file)
{
    if (file->handle == ROOT_DIRECTORY_HANDLE)
    {
        file->position = 0;
        FAT_Data.rootDirectory.currentCluster = FAT_Data.rootDirectory.firstCluster;
    }
    else
    {
        FAT_Data.openedFiles[file->handle].opened = false;
    }
}

void FAT_GetShortName(const char* name, char shortName[12])
{
    memset(shortName, ' ', 12);
    shortName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (!ext) ext = name + 11;

    for (uint32_t i = 0; i < 8 && name[i] && name + i < ext; i++)
        shortName[i] = toupper(name[i]);

    if (ext != name + 11)
    {
        for (uint32_t i = 0; i < 3 && ext[i + 1]; i++)
            shortName[i + 8] = toupper(ext[i + 1]);
    }
}

bool FAT_FindFile(Partition* partition, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    // TODO: LFN

    char shortName[12];
    FAT_GetShortName(name, shortName);

    FAT_DirectoryEntry entry;
    while (FAT_ReadEntry(partition, file, &entry))
    {
        if (entry.Name[0] == 0x00) // End of Directory
            break;
        if (entry.Name[0] == 0xE5) // Deleted entry
            continue;

        if (entry.Attributes == FAT_ATTRIBUTE_LFN)
            continue;

        if (memcmp(shortName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

FAT_File* FAT_Open(Partition* partition, const char* path)
{
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FAT_File* current = &FAT_Data.rootDirectory.public;

    while (*path)
    {
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim)
        {
            memcpy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        }
        else
        {
            uint32_t len = strlen(path);
            memcpy(name, path, len);
            name[len] = '\0';
            path += len;
            isLast = true;
        }

        FAT_DirectoryEntry entry;
        if (FAT_FindFile(partition, current, name, &entry))
        {
            FAT_Close(current);

            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                IO_PrintFormat(dbgout, "FAT: %s not a directory\n", name);
                return NULL;
            }

            current = FAT_OpenEntry(partition, &entry);
            if (!current)
            {
                IO_PrintFormat(dbgout, "FAT: Couldn't open %s\n", name);
                return NULL;
            }
        }
        else
        {
            FAT_Close(current);

            IO_PrintFormat(dbgout, "FAT: %s not found\n", name);
            return NULL;
        }
    }

    return current;
}
