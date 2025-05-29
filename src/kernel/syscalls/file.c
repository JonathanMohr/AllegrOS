#include "file.h"

#include "../drivers/fat/fat.h"
#include <stddef.h>

#define MAX_FD 15

Partition* partition;
FAT_File* fd_table[MAX_FD];

static FAT_File dummy_file;

void File_Init(Partition* part)
{
    partition = part;
    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i] = NULL;
    }

    // stdin/stdout/stderr/debug
    fd_table[0] = &dummy_file;
    fd_table[1] = &dummy_file;
    fd_table[2] = &dummy_file;
    fd_table[3] = &dummy_file;
}


int allocateFdForFile(FAT_File* file)
{
    for (int i = 0; i < MAX_FD; i++) {
        if (fd_table[i] == NULL) {
            fd_table[i] = file;
            return i; // Gefundener FD
        }
    }
    return -1; // Keine freie FD
}

FAT_File* getFile(uint32_t fd)
{
    if (fd < 0 || fd >= MAX_FD) return NULL;
    return fd_table[fd];
}

uint32_t File_Open(const char* path, uint32_t flags, uint32_t mode)
{
    //TODO: flags & mode
    FAT_File* fd = FAT_Open(partition, path);
    if (!fd)
    {
        return (uint32_t)-1;
    }
    return allocateFdForFile(fd);
}

uint32_t File_Close(uint32_t handle)
{
    FAT_File* fd = getFile(handle);
    if (!fd)
    {
        return 0;
    }
    FAT_Close(fd);
    return 1;
}

uint32_t File_Read(uint32_t handle, uint8_t* buffer, uint32_t count)
{
    if (handle < 4)
    {
        return (uint32_t)-1;
    }

    FAT_File* fd = getFile(handle);
    if (!fd)
    {
        return (uint32_t)-1;
    }
    
    return FAT_Read(partition, fd, count, buffer);
}