#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>

VFS_File* VFS_Dir_Open(VFS* vfs, VFS_Entry* wd, const char* path)
{
    VFS_Entry* entry = VFS_GetEntry(vfs, wd, path);
    if (!entry)
        return NULL;

    if ((entry->mount ? entry->rootNode : entry->node)->node.type != FILESYSTEM_ENTRY_DIRECTORY)
        return NULL;

    VFS_File* dir = Memory_KernelAllocate(sizeof(VFS_File));
    if (!dir)
        return NULL;

    if (!(entry->mount ? entry->rootNode : entry->node)->driver->openFile((entry->mount ? entry->rootNode : entry->node)->driver, &(entry->mount ? entry->rootNode : entry->node)->node, &dir->file))
    {
        Memory_KernelFree(dir);
        return NULL;
    }
    (entry->mount ? entry->rootNode : entry->node)->openHandleCount++;
    (entry->mount ? entry->rootNode : entry->node)->cacheReferences++;
    
    dir->entry = entry;
    dir->node = entry->mount ? entry->rootNode : entry->node;
    dir->vfs = vfs;

    return dir;
}

void VFS_Dir_Close(VFS_File* dir)
{
    if (dir->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
        return;

    dir->node->driver->closeFile(dir->node->driver, &dir->file);

    if (--dir->node->openHandleCount == 0 && dir->node->node.referenceCount == 0)
        (void)dir->node->driver->removeNode(dir->node->driver, &dir->node->node);

    VFS_PutNode(dir->vfs, dir->node);

    Memory_KernelFree(dir);
}

// TODO: Optimize
VFS_Entry* VFS_Dir_Read(VFS_File* dir)
{
    if (dir->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
        return NULL;

    Filesystem_Entry rawEntry;
    if (dir->node->driver->readEntry(dir->node->driver, &dir->file, &rawEntry) != FILESYSTEM_DIR_ENTRY_FOUND)
        return NULL;

    VFS_Entry* entry = VFS_GetEntry(dir->vfs, dir->entry, rawEntry.name);
    return entry;
}
