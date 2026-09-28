#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>

VFS_File* VFS_File_Open(VFS* vfs, VFS_Entry* wd, const char* path)
{
    VFS_Entry* entry = VFS_GetEntry(vfs, wd, path);
    if (!entry)
        return NULL;

    if ((entry->mount ? entry->rootNode : entry->node)->node.type != FILESYSTEM_ENTRY_FILE)
        return NULL;

    VFS_File* file = Memory_KernelAllocate(sizeof(VFS_File));
    if (!file)
        return NULL;

    if (!(entry->mount ? entry->rootNode : entry->node)->driver->openFile(((entry->mount ? entry->rootNode : entry->node))->driver, &((entry->mount ? entry->rootNode : entry->node))->node, &file->file))
    {
        Memory_KernelFree(file);
        return NULL;
    }
    entry->node->openHandleCount++;
    entry->node->cacheReferences++;
    
    file->entry = NULL;
    file->node = entry->mount ? entry->rootNode : entry->node;
    file->vfs = vfs;

    return file;
}

void VFS_File_Close(VFS_File* file)
{
    if (file->node->node.type != FILESYSTEM_ENTRY_FILE)
        return;

    file->node->driver->closeFile(file->node->driver, &file->file);

    if (--file->node->openHandleCount == 0 && file->node->node.referenceCount == 0)
    {
        (void)file->node->driver->removeNode(file->node->driver, &file->node->node);
        file->node->driver->cleanupNode(file->node->driver, &file->node->node);
    }

    VFS_PutNode(file->vfs, file->node);

    Memory_KernelFree(file);
}

uint64_t VFS_File_Read(VFS_File* file, uint64_t size, void* buffer)
{
    if (file->node->node.type != FILESYSTEM_ENTRY_FILE)
        return 0;

    return file->node->driver->read(file->node->driver, &file->file, size, buffer);
}

uint64_t VFS_File_Write(VFS_File* file, uint64_t size, const void* buffer)
{
    if (file->node->node.type != FILESYSTEM_ENTRY_FILE)
        return 0;

    return file->node->driver->write(file->node->driver, &file->file, size, buffer);
}

bool VFS_File_Seek(VFS_File* file, uint64_t pos)
{
    if (file->node->node.type != FILESYSTEM_ENTRY_FILE)
        return false;

    return file->node->driver->seek(file->node->driver, &file->file, pos);
}

uint64_t VFS_File_Tell(VFS_File* file)
{
    if (file->node->node.type != FILESYSTEM_ENTRY_FILE)
        return VFS_TELL_ERROR;

    return file->file.pos;
}
