#include "fat.h"

#include <core/string/string.h>
#include <core/string/ctype.h>
#include <core/minmax.h>
#include <core/memory/memory.h>

#include "../disk/mbr.h"
#include <stddef.h>
#include "fatdefs.h"
#include "../../debug.h"
#include "../../memory/memory.h"

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

FAT_File* FAT_OpenEntry(Partition* disk, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        log_err("FAT", "Out of file handles.");
        return false;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!Partition_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        log_err("FAT", "Open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
        for (int i = 0; i < 11; i++)
            log_err("FAT", "%c", entry->Name[i]);
        return false;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT_NextCluster(Partition* disk, uint32_t currentCluster)
{
    // Determine the byte offset of the entry we need to read
    uint32_t fatIndex;
    if (g_FatType == 12) {
        fatIndex = currentCluster * 3 / 2;
    }
    else if (g_FatType == 16) {
        fatIndex = currentCluster * 2;
    }
    else /*if (g_FatType == 32)*/ {
        fatIndex = currentCluster * 4;
    }

    // Make sure cache has the right number
    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if (fatIndexSector < g_Data->FatCachePosition
        || fatIndexSector >= g_Data->FatCachePosition + FAT_CACHE_SIZE)
    {
        FAT_ReadFat(disk, fatIndexSector);
        g_Data->FatCachePosition = fatIndexSector;
    }

    fatIndex -= (g_Data->FatCachePosition * SECTOR_SIZE);

    uint32_t nextCluster;
    if (g_FatType == 12) {
        if (currentCluster % 2 == 0)
            nextCluster = (*(uint16_t*)(g_Data->FatCache + fatIndex)) & 0x0FFF;
        else
            nextCluster = (*(uint16_t*)(g_Data->FatCache + fatIndex)) >> 4;
        
        if (nextCluster >= 0xFF8) {
            nextCluster |= 0xFFFFF000;
        }
    }
    else if (g_FatType == 16) {
        nextCluster = *(uint16_t*)(g_Data->FatCache + fatIndex);
        if (nextCluster >= 0xFFF8) {
            nextCluster |= 0xFFFF0000;
        }
    }
    else /*if (g_FatType == 32)*/ {
        nextCluster = *(uint32_t*)(g_Data->FatCache + fatIndex);
    }

    return nextCluster;
}

uint32_t FAT_Read(Partition* disk, FAT_File* file, uint32_t byteCount, void* dataOut)
{
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
        ? g_Data->RootDirectory
        : &g_Data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0))
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        // log_debug("FAT", "leftInBuffer=%lu, take=%lu.", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // Special handling for root directory
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                // read next sector
                if (!Partition_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    log_err("FAT", "Read error!");
                    break;
                }
            }
            else
            {
                // calculate next cluster & sector to read
                if (++fd->CurrentSectorInCluster >= g_Data->BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(disk, fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFFFFFFF8)
                {
                    // Mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!Partition_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    log_err("FAT", "Read error!");
                    break;
                }
            }
        }
    }
    
    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT_ReadEntry(Partition* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        g_Data->RootDirectory->CurrentCluster = g_Data->RootDirectory->FirstCluster;
    }
    else
    {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}

void FAT_GetShortName(const char* name, char shortName[12])
{
    // convert from name to fat name
    memset(shortName, ' ', 12);
    shortName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
    shortName[i] = toupper(name[i]);

    if (ext != name + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
        shortName[i + 8] = toupper(ext[i + 1]);
    }
}

bool FAT_FindFile(Partition* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char shortName[12];
    //char longName[256];
    FAT_DirectoryEntry entry;

    FAT_GetShortName(name, shortName);

    uint64_t i = 0;

    while (FAT_ReadEntry(disk, file, &entry))
    {
        // Ende des Verzeichnisses?
        if (entry.Name[0] == 0x00) {
            // Kein weiterer Eintrag -> Datei nicht gefunden
            break;
        }
        // Gelöschter Eintrag?
        if (entry.Name[0] == 0xE5) {
            // überspringen
            continue;
        }

        /*
        if (entry.Attributes == FAT_ATTRIBUTE_LFN) {
            FAT_LongFileEntry* lfn = (FAT_LongFileEntry*)&entry;

            int idx = g_Data->LFNCount++;
            g_Data->LFNBlocks[idx].Order = lfn->Order & (FAT_LFN_LAST - 1);
            memcpy(g_Data->LFNBlocks[idx].Chars, lfn->Chars1, sizeof(lfn->Chars1));
            memcpy(g_Data->LFNBlocks[idx].Chars + 5, lfn->Chars2, sizeof(lfn->Chars2));
            memcpy(g_Data->LFNBlocks[idx].Chars + 11, lfn->Chars1, sizeof(lfn->Chars3));

            // is this the last LFN block
            if ((lfn->Order & FAT_LFN_LAST) != 0) {
                qsort(g_Data->LFNBlocks, g_Data->LFNCount, sizeof(FAT_LFNBlock), FAT_CompareLFNBlocks);
                char* namePos = longName;
                for (int i = 0; i < g_Data->LFNCount; i++)
                {
                    int16_t* chars = g_Data->LFNBlocks[i].Chars;
                    int16_t* charsLimit = chars + 13;

                    while (chars < charsLimit && *chars != 0)
                    {
                        int codepoint;
                        chars = utf16_to_codepoint(chars, &codepoint);
                        namePos = codepoint_to_utf8(codepoint, namePos);
                    }
                }
                *namePos = 0;
                // REMOVE IN FUTURE
                printf("LFN: %s\n", longName);
            }
        }
        */

        if (memcmp(shortName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

FAT_File* FAT_Open(Partition* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data->RootDirectory->Public;

    while (*path)
    {
        // extract next file name from path
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len] = '\0';
            path += len;
            isLast = true;
        }

        // find directory entry in current directory
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            FAT_Close(current);

            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0)
            {
                log_err("FAT", "%s not a directory.", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(disk, &entry);
        }
        else
        {
            FAT_Close(current);

            log_err("FAT", "%s not found.", name);
            return NULL;
        }
    }

    return current;
}