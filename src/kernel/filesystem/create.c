#include "filesystem.h"
#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>
#include <memory.h>

bool VFS_Create(VFS* vfs, VFS_Entry* wd, const char* path, Filesystem_Entry_Type type, Filesystem_Entry_Attribute attributes)
{
    if (!wd)
        wd = &vfs->root;

    const char* pathStart = path;
    const char* lastDelimiter = NULL;
    const char* tmpCheck = NULL;
    while (*path)
    {
        if (*path == '/')
            tmpCheck = path;
        else
            lastDelimiter = tmpCheck;
        path++;
    }

    const uint64_t parentPathLength = lastDelimiter ? lastDelimiter - pathStart : 0;
    char* parentPath = parentPathLength ? Memory_KernelAllocate(parentPathLength + 1) : NULL;
    if (parentPathLength && !parentPath)
        return false;
    if (parentPathLength)
    {
        memcpy(parentPath, pathStart, parentPathLength);
        parentPath[parentPathLength] = '\0';
    }

    VFS_Entry* parentEntry = wd;
    if (parentPathLength)
    {
        parentEntry = VFS_GetEntry(vfs, wd, parentPath);
        Memory_KernelFree(parentPath);
        if (!parentEntry || parentEntry->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
            return false;
    }

    const char* name = lastDelimiter ? lastDelimiter + 1 : pathStart;
    VFS_Entry* shouldNotExist = VFS_GetEntry(vfs, parentEntry, name);
    if (shouldNotExist)
        return false;

    Filesystem_Node outNode;
    if (!parentEntry->node->driver->createNode(parentEntry->node->driver, &parentEntry->node->node, type, attributes, name, &outNode))
        return false;

    parentEntry->node->driver->cleanupNode(parentEntry->node->driver, &outNode);

    // TODO: Optimization: Add to tree to directly

    return true;
}
