#pragma once

#include "filesystem.h"
#include <stdint.h>

typedef struct
{
    uint64_t openHandleCount;
    uint64_t cacheReferences;

    Filesystem_Driver* driver;

    Filesystem_Node node;
} VFS_Node;

typedef struct
{
    Filesystem_Driver* driver;

    Filesystem_Entry entry;
} VFS_Entry;


typedef struct VFS_Node_Cache_Entry
{
    Filesystem_Driver* driver;
    uint64_t number;

    VFS_Node* node;

    struct VFS_Node_Cache_Entry* next;
} VFS_Node_Cache_Entry;

typedef struct VFS_Node_Cache
{
    VFS_Node_Cache_Entry** buckets;
    uint64_t bucketCount;
    uint64_t entryCount;
} VFS_Node_Cache;

uint64_t VFS_Cache_HashNode(Filesystem_Driver* driver, uint64_t number);
VFS_Node* VFS_Cache_Lookup(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number);
bool VFS_Cache_Insert(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number, VFS_Node* node);
void VFS_Cache_Remove(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number);


typedef struct
{
    VFS_Node_Cache* nodeCache;
} VFS;

VFS_Node* VFS_GetNode(VFS* vfs, Filesystem_Driver* driver, uint64_t number);
void VFS_PutNode(VFS* vfs, VFS_Node* node);
