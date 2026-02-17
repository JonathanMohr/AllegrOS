#include "file.h"

#include "../drivers/fat/fat.h"
#include "../drivers/keyboard/keyboard.h"
#include <stddef.h>
#include "../hal/vfs.h"

#include "debug.h"

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

int32_t File_Open(const char* path, uint32_t flags, uint32_t mode)
{
    //TODO: flags & mode
    FAT_File* file = FAT_Open(partition, path);
    if (!file)
    {
        return -1;
    }
    return allocateFdForFile(file);
}

int32_t File_Close(uint32_t fd)
{
    FAT_File* file = getFile(fd);
    if (!file)
    {
        return -1;
    }
    FAT_Close(file);
    return 0;
}

int32_t File_Read(uint32_t fd, uint8_t* buffer, uint32_t count)
{
    if (fd == 0) // stdin = evdev Events
    {
        uint32_t max_events = count / sizeof(InputEvent);
        uint32_t read_events = InputBuffer_Read((InputEvent*)buffer, max_events);
        return read_events * sizeof(InputEvent);
    }
    else if (fd < 4)
    {
        return -1;
    }

    FAT_File* file = getFile(fd);
    if (!file)
    {
        return -1;
    }
    
    return FAT_Read(partition, file, count, buffer);
}

int32_t File_Write(uint32_t fd, uint8_t* buffer, uint32_t count)
{
    if (fd < 4)
    {
        // stdin, stdout, stderr, stddebug
        return VFS_Write(fd, buffer, count);
    }
    
    FAT_File* file = getFile(fd);
    if (!file)
    {
        return (uint32_t)-1;
    }

    //TODO
    return -1;
}

int64_t File_Seek(uint32_t fd, int64_t offset, uint32_t whence)
{
    if (fd < 1)
    {
        return -1;
    }
    else if (fd < 4)
    {
        return -1;
    }

    FAT_File* file = getFile(fd);
    if (!file)
    {
        return -1;
    }

    int64_t newPos = FAT_Seek(partition, file, offset, whence);

    return newPos;
}