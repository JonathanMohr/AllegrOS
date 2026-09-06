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

    // Root directory

    /* entry does not need to be filled */
    vfs->root.node = NULL;
    vfs->root.parent = NULL;
    vfs->root.nextSibling = NULL;
    vfs->root.firstChild = NULL;
    vfs->root.rootNode = VFS_GetNode(vfs, rootMount, rootMount->rootNodeNumber);
    vfs->root.mount = true;
    if (!vfs->root.rootNode)
    {
        Memory_KernelFree(vfs->nodeCache.buckets);
        return false;
    }

    vfs->root.childCount = rootMount->getEntryCount(rootMount, &vfs->root.rootNode->node);
    if (vfs->root.childCount == FILESYSTEM_ENTRY_COUNT_ERROR)
    {
        VFS_PutNode(vfs, vfs->root.rootNode);
        Memory_KernelFree(vfs->nodeCache.buckets);
        return false;
    }

    return true;
}

void VFS_Destroy(VFS* vfs)
{
    // TODO: Clear every node

    // TODO: basically umount root and destroy driver
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

VFS_Entry* VFS_GetEntry(VFS* vfs, VFS_Entry* wd, const char* path)
{
    if (path[0] == '/' || !wd)
        wd = &vfs->root;

    while (*path == '/')
        path++;

    uint64_t nameOffset;
    char nameBuffer[FILESYSTEM_MAX_NAME + 1];
    while (*path)
    {
        nameOffset = 0;
        while (*path && *path != '/' && nameOffset < FILESYSTEM_MAX_NAME)
            nameBuffer[nameOffset++] = *path++;
        if (*path && *path != '/')
            return NULL;
        if (*path == '/') path++;
        nameBuffer[nameOffset] = '\0';

        const uint64_t nameLen = nameOffset;

        if (nameLen == 1 && nameBuffer[0] == '.')
            continue;

        if (nameLen == 2 && nameBuffer[0] == '.' && nameBuffer[1] == '.')
        {
            if (wd->parent)
                wd = wd->parent;
            continue;
        }

        VFS_Node* currentNode = wd->mount ? wd->rootNode : wd->node;

        bool found = false;
        VFS_Entry* lastSibling = NULL;
        VFS_Entry* currentSibling = wd->firstChild;
        while (currentSibling)
        {
            bool equal = true;
            for (uint64_t i = 0; i < nameLen + 1 && equal; i++)
            {
                char c1 = nameBuffer[i];
                char c2 = currentSibling->name[i];
                if (!currentNode->driver->caseSensitive)
                {
                    if (c1 >= 'a' && c1 <= 'z')
                        c1 = c1 - 'a' + 'A';
                    if (c2 >= 'a' && c2 <= 'z')
                        c2 = c2 - 'a' + 'A';
                }
                if (c1 != c2)
                    equal = false;
            }
            if (equal)
            {
                found = true;
                break;
            }

            lastSibling = currentSibling;
            currentSibling = currentSibling->nextSibling;
        }

        if (!found)
        {
            Filesystem_Entry entryBuffer;
            if (currentNode->driver->lookupEntry(currentNode->driver, &currentNode->node, nameBuffer, &entryBuffer) != FILESYSTEM_DIR_ENTRY_FOUND)
                return NULL;

            VFS_Entry* newEntry = Memory_KernelAllocate(sizeof(VFS_Entry));
            if (!newEntry)
                return NULL;

            memcpy(newEntry->name, nameBuffer, nameLen);
            newEntry->name[nameLen] = '\0';

            newEntry->childCount = 0; // TODO

            newEntry->node = VFS_GetNode(vfs, currentNode->driver, entryBuffer.node);
            if (!newEntry->node)
            {
                Memory_KernelFree(newEntry);
                return NULL;
            }
            newEntry->parent = wd;

            newEntry->nextSibling = NULL;
            newEntry->firstChild = NULL;

            newEntry->rootNode = NULL;
            newEntry->mount = false;

            if (newEntry->node->node.type == FILESYSTEM_ENTRY_DIRECTORY)
            {
                newEntry->childCount = currentNode->driver->getEntryCount(currentNode->driver, &newEntry->node->node);
                if (newEntry->childCount == FILESYSTEM_ENTRY_COUNT_ERROR)
                {
                    Memory_KernelFree(newEntry);
                    VFS_PutNode(vfs, newEntry->node);
                    return NULL;
                }
            }

            if (lastSibling)
                lastSibling->nextSibling = newEntry;
            else
                wd->firstChild = newEntry;

            currentSibling = newEntry;
        }

        wd = currentSibling;
    }

    return wd;
}
