#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>

VFS_Node* VFS_GetNode(VFS* vfs, Filesystem_Driver* driver, uint64_t number)
{
    VFS_Node* n = VFS_Cache_Lookup(vfs->nodeCache, driver, number);
    if (n)
    {
        n->cacheReferences++;
        return n;
    }

    VFS_Node* node = Memory_KernelAllocate(sizeof(VFS_Node));
    if (!node)
        return NULL;
    node->openHandleCount = 0;
    node->cacheReferences = 1;
    node->driver = driver;
    if (!driver->getNode(driver, number, &node->node))
    {
        Memory_KernelFree(node);
        return NULL;
    }

    if (!VFS_Cache_Insert(vfs->nodeCache, driver, number, node))
    {
        driver->cleanupNode(driver, &node->node);
        Memory_KernelFree(node);
        return NULL;
    }

    return node;
}

void VFS_PutNode(VFS* vfs, VFS_Node* node)
{
    Filesystem_Driver* driver = node->driver;
    const uint64_t number = node->node.number;

    if (node->cacheReferences-- <= 1)
    {
        VFS_Cache_Remove(vfs->nodeCache, driver, number);
        driver->cleanupNode(driver, &node->node);
        Memory_KernelFree(node);
    }
}
