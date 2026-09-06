#pragma once

#include "filesystem.h"
#include <stdint.h>

typedef struct VFS_Node
{
    uint64_t openHandleCount;
    uint64_t cacheReferences;

    Filesystem_Driver* driver;

    Filesystem_Node node;
} VFS_Node;

typedef struct VFS_Entry
{
    // if directory
    uint64_t childCount;

    char name[FILESYSTEM_MAX_NAME + 1];
    VFS_Node* node;
    struct VFS_Entry* parent;

    struct VFS_Entry* nextSibling;

    // if directory
    struct VFS_Entry* firstChild;

    // if mount
    VFS_Node* rootNode;

    bool mount;
} VFS_Entry;

typedef struct VFS_File
{
    Filesystem_File file;
    Filesystem_Driver* driver;
    struct VFS* vfs;
} VFS_File;


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
    VFS_Node_Cache nodeCache;

    VFS_Entry root;
} VFS;

bool VFS_Initialize(VFS* vfs, Filesystem_Driver* rootMount);
void VFS_Destroy(VFS* vfs);

VFS_Node* VFS_GetNode(VFS* vfs, Filesystem_Driver* driver, uint64_t number);
void VFS_PutNode(VFS* vfs, VFS_Node* node);


VFS_File* VFS_File_Open(VFS* vfs, VFS_Entry* wd, const char* path);
void VFS_File_Close(VFS_File* file);
VFS_File* VFS_File_Read(VFS_File* file, uint64_t size, void* buffer);
VFS_File* VFS_File_Write(VFS_File* file, uint64_t size, const void* buffer);
bool VFS_File_Seek(VFS_File* file, uint64_t pos);
uint64_t VFS_File_Tell(VFS_File* file);

VFS_File* VFS_Dir_Open(VFS* vfs, VFS_Entry* wd, const char* path);
void VFS_Dir_Close(VFS_File* dir);
bool VFS_Dir_Read(VFS_File* dir, VFS_Entry* out);

bool VFS_Create(VFS* vfs, VFS_Entry* wd, const char* path, Filesystem_Entry_Type type, Filesystem_Entry_Attribute attributes);
/** Will change node in cache, hardlinks and open handles */
bool VFS_Move(VFS* vfs, VFS_Entry* wd, const char* oldPath, const char* newPath);

bool VFS_Link(VFS* vfs, VFS_Entry* wd, const char* path, const char* targetPath);
bool VFS_Unlink(VFS* vfs, VFS_Entry* wd, const char* path);
