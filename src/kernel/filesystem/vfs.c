#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>
#include <memory.h>

bool VFS_Initialize(VFS* vfs, Filesystem_Driver* rootMount)
{
    const uint64_t bucketCount = 64;
    vfs->nodeCache.buckets = Memory_KernelAllocate(bucketCount * sizeof(VFS_Node_Cache_Entry*));
    if (!vfs->nodeCache.buckets)
        return false;
    vfs->nodeCache.bucketCount = bucketCount;
    vfs->nodeCache.entryCount = 0;

    memset(vfs->root.entry.name, 0, sizeof(vfs->root.entry.name));
    vfs->root.entry.node = 0;
    vfs->root.node = NULL;
    vfs->root.parent = NULL;
    vfs->root.nextSibling = NULL;
    vfs->root.firstChild = NULL;
    vfs->root.rootNode = Memory_KernelAllocate(sizeof(VFS_Node));
    vfs->root.mount = true;
    if (!vfs->root.rootNode)
    {
        Memory_KernelFree(vfs->nodeCache.buckets);
        return false;
    }

    vfs->root.rootNode->openHandleCount = 0;
    vfs->root.rootNode->cacheReferences = 1;
    vfs->root.rootNode->driver = rootMount;
    if (!rootMount->getRoot(rootMount, &vfs->root.rootNode->node))
    {
        Memory_KernelFree(vfs->root.rootNode);
        Memory_KernelFree(vfs->nodeCache.buckets);
        return false;
    }

    vfs->root.childCount = rootMount->getEntryCount(rootMount, &vfs->root.rootNode->node);
    if (vfs->root.childCount == FILESYSTEM_ENTRY_COUNT_ERROR)
    {
        rootMount->cleanupNode(rootMount, &vfs->root.rootNode->node);
        Memory_KernelFree(vfs->root.rootNode);
        Memory_KernelFree(vfs->nodeCache.buckets);
        return false;
    }

    return true;
}

void VFS_Destroy(VFS* vfs)
{
    // TODO: Clear every node
    
}


VFS_Node* VFS_GetNode(VFS* vfs, Filesystem_Driver* driver, uint64_t number)
{
    VFS_Node* n = VFS_Cache_Lookup(&vfs->nodeCache, driver, number);
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

    if (!VFS_Cache_Insert(&vfs->nodeCache, driver, number, node))
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
        VFS_Cache_Remove(&vfs->nodeCache, driver, number);
        driver->cleanupNode(driver, &node->node);
        Memory_KernelFree(node);
    }
}
