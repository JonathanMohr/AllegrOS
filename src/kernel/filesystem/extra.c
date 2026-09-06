#include "filesystem.h"
#include "vfs.h"
#include "../memory/memory.h"
#include <stddef.h>
#include <memory.h>

static bool VFS_IsAncestor(VFS_Entry* potentialAncestor, VFS_Entry* entry)
{
    VFS_Entry* current = entry;
    while (current)
    {
        if (current == potentialAncestor)
            return true;
        current = current->parent;
    }
    return false;
}

static uint64_t VFS_CopyMoveNode(Filesystem_Driver* srcDriver, Filesystem_Node* srcParentNode, const char* srcName, Filesystem_Node* srcNode,
                                  Filesystem_Driver* dstDriver, Filesystem_Node* dstParentNode, const char* dstName)
{
    if (srcNode->type == FILESYSTEM_ENTRY_DIRECTORY)
    {
        Filesystem_Node newDirNode;
        if (!dstDriver->createNode(dstDriver, dstParentNode, FILESYSTEM_ENTRY_DIRECTORY, srcNode->attributes, dstName, &newDirNode))
            return FILESYSTEM_MOVE_ERROR;

        Filesystem_File dirFile;
        if (!srcDriver->openFile(srcDriver, srcNode, &dirFile))
        {
            dstDriver->cleanupNode(dstDriver, &newDirNode);
            return FILESYSTEM_MOVE_ERROR;
        }

        while (1)
        {
            Filesystem_Entry rawEntry;
            uint8_t status = srcDriver->readEntry(srcDriver, &dirFile, &rawEntry);
            if (status == FILESYSTEM_DIR_END)
                break;
            if (status != FILESYSTEM_DIR_ENTRY_FOUND)
            {
                srcDriver->closeFile(srcDriver, &dirFile);
                dstDriver->cleanupNode(dstDriver, &newDirNode);
                return FILESYSTEM_MOVE_ERROR;
            }

            if (rawEntry.name[0] == '.' && (rawEntry.name[1] == '\0' ||
                (rawEntry.name[1] == '.' && rawEntry.name[2] == '\0')))
                continue;

            Filesystem_Node childNode;
            if (!srcDriver->getNode(srcDriver, rawEntry.node, &childNode))
            {
                srcDriver->closeFile(srcDriver, &dirFile);
                dstDriver->cleanupNode(dstDriver, &newDirNode);
                return FILESYSTEM_MOVE_ERROR;
            }

            uint64_t childResult = VFS_CopyMoveNode(srcDriver, srcNode, rawEntry.name, &childNode,
                                                      dstDriver, &newDirNode, rawEntry.name);

            srcDriver->cleanupNode(srcDriver, &childNode);

            if (childResult == FILESYSTEM_MOVE_ERROR)
            {
                srcDriver->closeFile(srcDriver, &dirFile);
                dstDriver->cleanupNode(dstDriver, &newDirNode);
                return FILESYSTEM_MOVE_ERROR;
            }
        }

        srcDriver->closeFile(srcDriver, &dirFile);

        uint64_t hardlinksLeft = srcDriver->unlink(srcDriver, srcParentNode, srcName);
        if (hardlinksLeft == FILESYSTEM_UNLINK_ERROR)
        {
            dstDriver->cleanupNode(dstDriver, &newDirNode);
            return FILESYSTEM_MOVE_ERROR;
        }
        if (hardlinksLeft == 0)
            (void)srcDriver->removeNode(srcDriver, srcNode);

        uint64_t result = newDirNode.number;
        dstDriver->cleanupNode(dstDriver, &newDirNode);
        return result;
    }
    else
    {
        Filesystem_Node newFileNode;
        if (!dstDriver->createNode(dstDriver, dstParentNode, FILESYSTEM_ENTRY_FILE, srcNode->attributes, dstName, &newFileNode))
            return FILESYSTEM_MOVE_ERROR;

        Filesystem_File srcFile, dstFile;
        if (!srcDriver->openFile(srcDriver, srcNode, &srcFile))
        {
            dstDriver->cleanupNode(dstDriver, &newFileNode);
            return FILESYSTEM_MOVE_ERROR;
        }
        if (!dstDriver->openFile(dstDriver, &newFileNode, &dstFile))
        {
            srcDriver->closeFile(srcDriver, &srcFile);
            dstDriver->cleanupNode(dstDriver, &newFileNode);
            return FILESYSTEM_MOVE_ERROR;
        }

        uint8_t buffer[4096];
        uint64_t remaining = srcNode->size;
        bool ok = true;

        while (remaining > 0)
        {
            uint64_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            uint64_t bytesRead = srcDriver->read(srcDriver, &srcFile, chunk, buffer);
            if (bytesRead == 0)
            {
                ok = false;
                break;
            }
            uint64_t bytesWritten = dstDriver->write(dstDriver, &dstFile, bytesRead, buffer);
            if (bytesWritten != bytesRead)
            {
                ok = false;
                break;
            }
            remaining -= bytesRead;
        }

        srcDriver->closeFile(srcDriver, &srcFile);
        dstDriver->closeFile(dstDriver, &dstFile);

        if (!ok)
        {
            (void)dstDriver->unlink(dstDriver, dstParentNode, dstName);
            dstDriver->cleanupNode(dstDriver, &newFileNode);
            return FILESYSTEM_MOVE_ERROR;
        }

        uint64_t hardlinksLeft = srcDriver->unlink(srcDriver, srcParentNode, srcName);
        if (hardlinksLeft == FILESYSTEM_UNLINK_ERROR)
        {
            dstDriver->cleanupNode(dstDriver, &newFileNode);
            return FILESYSTEM_MOVE_ERROR;
        }
        if (hardlinksLeft == 0)
            (void)srcDriver->removeNode(srcDriver, srcNode);

        uint64_t result = newFileNode.number;
        dstDriver->cleanupNode(dstDriver, &newFileNode);
        return result;
    }
}

static uint64_t VFS_CopyMove(VFS_Entry* old, VFS_Entry* newParent, const char* newName)
{
    VFS_Node* oldParentDir = old->parent->mount ? old->parent->rootNode : old->parent->node;
    VFS_Node* newParentDir = newParent->mount ? newParent->rootNode : newParent->node;

    return VFS_CopyMoveNode(old->node->driver, &oldParentDir->node, old->name, &old->node->node,
                             newParentDir->driver, &newParentDir->node, newName);
}

bool VFS_MoveEntry(VFS* vfs, VFS_Entry* wd, const char* oldPath, const char* newPath, bool allowCrossMounts)
{
    if (!wd)
        wd = &vfs->root;

    const char* oldLastDelimiter = VFS_GetLastDelimiter(oldPath);
    const char* newLastDelimiter = VFS_GetLastDelimiter(newPath);

    const uintptr_t oldParentPathLen = (uintptr_t)oldLastDelimiter - (uintptr_t)oldPath;
    const uintptr_t newParentPathLen = (uintptr_t)newLastDelimiter - (uintptr_t)newPath;

    char* oldParentPath = oldParentPathLen ? Memory_KernelAllocate(oldParentPathLen + 1) : NULL;
    if (oldParentPathLen)
    {
        if (!oldParentPath) return false;
        memcpy(oldParentPath, oldPath, oldParentPathLen);
        oldParentPath[oldParentPathLen] = '\0';
    }
    char* newParentPath = newParentPathLen ? Memory_KernelAllocate(newParentPathLen + 1) : NULL;
    if (newParentPathLen)
    {
        if (!newParentPath)
        {
            if (oldParentPathLen) Memory_KernelFree(oldParentPath);
            return false;
        }
        memcpy(newParentPath, newPath, newParentPathLen);
        newParentPath[newParentPathLen] = '\0';
    }

    const char* oldName = VFS_GetName(oldPath, oldLastDelimiter);
    const char* newName = VFS_GetName(newPath, newLastDelimiter);

    VFS_Entry* oldParentEntry = wd;
    if (oldParentPathLen)
    {
        oldParentEntry = VFS_GetEntry(vfs, wd, oldParentPath);
        Memory_KernelFree(oldParentPath);
        if (!oldParentEntry || oldParentEntry->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
        {
            if (newParentPathLen) Memory_KernelFree(newParentPath);
            return false;
        }
    }
    VFS_Entry* newParentEntry = wd;
    if (newParentPathLen)
    {
        newParentEntry = VFS_GetEntry(vfs, wd, newParentPath);
        Memory_KernelFree(newParentPath);
        if (!newParentEntry || newParentEntry->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
            return false;
    }

    VFS_Entry* oldEntry = VFS_GetEntry(vfs, oldParentEntry, oldName);
    if (!oldEntry) return false;
    if (oldEntry->mount) return false;

    VFS_Entry* newEntry = VFS_GetEntry(vfs, newParentEntry, newName);
    if (newEntry) return false;

    if (VFS_IsAncestor(oldEntry, newParentEntry)) return false;

    VFS_Node* oldParentDir = oldEntry->parent->mount ? oldEntry->parent->rootNode : oldEntry->parent->node;
    VFS_Node* newParentDir = newParentEntry->mount ? newParentEntry->rootNode : newParentEntry->node;

    uint64_t newNodeNumber;
    if (oldEntry->node->driver != newParentDir->driver)
    {
        if (allowCrossMounts)
            newNodeNumber = VFS_CopyMove(oldEntry, newParentEntry, newName);
        else
            return false;
    }
    else
        newNodeNumber = oldEntry->node->driver->move(oldEntry->node->driver, &oldParentDir->node, oldName, &newParentDir->node, newName);

    if (newNodeNumber == FILESYSTEM_MOVE_ERROR)
        return false;

    VFS_UnlinkEntryFromParent(oldEntry->parent, oldEntry);
    // Cannot fail, just returns false when old entry is not found
    (void)VFS_Cache_Rekey(&vfs->nodeCache, oldEntry->node->driver, oldEntry->node->node.number, newParentDir->driver, newNodeNumber);
    oldEntry->node->node.number = newNodeNumber;
    oldEntry->node->driver = newParentDir->driver;
    VFS_PutNode(vfs, oldEntry->node);
    Memory_KernelFree(oldEntry);

    // TODO: Optimization: Add to tree to directly

    return true;
}

bool VFS_Link(VFS* vfs, VFS_Entry* wd, const char* path, const char* targetPath)
{
    if (!wd)
        wd = &vfs->root;

    VFS_Entry* targetEntry = VFS_GetEntry(vfs, wd, targetPath);
    if (!targetEntry) return false;
    if (targetEntry->node->node.type != FILESYSTEM_ENTRY_FILE) return false;

    const char* lastDelimiter = VFS_GetLastDelimiter(path);
    const uintptr_t parentPathLen = (uintptr_t)lastDelimiter - (uintptr_t)path;

    char* parentPath = parentPathLen ? Memory_KernelAllocate(parentPathLen + 1) : NULL;
    if (parentPathLen)
    {
        if (!parentPath) return false;
        memcpy(parentPath, path, parentPathLen);
        parentPath[parentPathLen] = '\0';
    }

    const char* name = VFS_GetName(path, lastDelimiter);

    VFS_Entry* parentEntry = wd;
    if (parentPathLen)
    {
        parentEntry = VFS_GetEntry(vfs, wd, parentPath);
        Memory_KernelFree(parentPath);
        if (!parentEntry || parentEntry->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
            return false;
    }

    VFS_Node* targetDir = targetEntry->mount ? targetEntry->rootNode : targetEntry->node;
    VFS_Node* parentDir = parentEntry->mount ? parentEntry->rootNode : parentEntry->node;

    if (targetDir->driver != parentDir->driver)
        return false;

    VFS_Entry* existing = VFS_GetEntry(vfs, parentEntry, name);
    if (existing) return false;

    return parentDir->driver->link(parentDir->driver, &parentDir->node, name, &targetDir->node);
}

bool VFS_Unlink(VFS* vfs, VFS_Entry* wd, const char* path)
{
    if (!wd)
        wd = &vfs->root;

    const char* lastDelimiter = VFS_GetLastDelimiter(path);
    const uintptr_t parentPathLen = (uintptr_t)lastDelimiter - (uintptr_t)path;

    char* parentPath = parentPathLen ? Memory_KernelAllocate(parentPathLen + 1) : NULL;
    if (parentPathLen)
    {
        if (!parentPath) return false;
        memcpy(parentPath, path, parentPathLen);
        parentPath[parentPathLen] = '\0';
    }

    const char* name = VFS_GetName(path, lastDelimiter);

    VFS_Entry* parentEntry = wd;
    if (parentPathLen)
    {
        parentEntry = VFS_GetEntry(vfs, wd, parentPath);
        Memory_KernelFree(parentPath);
        if (!parentEntry || parentEntry->node->node.type != FILESYSTEM_ENTRY_DIRECTORY)
            return false;
    }

    VFS_Entry* entry = VFS_GetEntry(vfs, parentEntry, name);
    if (!entry) return false;
    if (entry->mount) return false;

    VFS_Node* parentDir = parentEntry->mount ? parentEntry->rootNode : parentEntry->node;

    if (entry->node->node.type == FILESYSTEM_ENTRY_DIRECTORY)
    {
        uint64_t entryCount = entry->node->driver->getEntryCount(entry->node->driver, &entry->node->node);
        if (entryCount == FILESYSTEM_ENTRY_COUNT_ERROR)
            return false;
        if (entryCount > 2)
            return false;
    }

    uint64_t hardlinksLeft = entry->node->driver->unlink(entry->node->driver, &parentDir->node, name);
    if (hardlinksLeft == FILESYSTEM_UNLINK_ERROR)
        return false;

    entry->node->node.referenceCount = hardlinksLeft;

    if (hardlinksLeft == 0 && entry->node->openHandleCount == 0)
        (void)entry->node->driver->removeNode(entry->node->driver, &entry->node->node);

    VFS_UnlinkEntryFromParent(parentEntry, entry);
    VFS_PutNode(vfs, entry->node);
    Memory_KernelFree(entry);

    return true;
}


void VFS_UnlinkEntryFromParent(VFS_Entry* parent, VFS_Entry* entry)
{
    if (parent->firstChild == entry)
    {
        parent->firstChild = entry->nextSibling;
        return;
    }

    VFS_Entry* current = parent->firstChild;
    while (current)
    {
        if (current->nextSibling == entry)
        {
            current->nextSibling = entry->nextSibling;
            return;
        }
        current = current->nextSibling;
    }
}
