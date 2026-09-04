#pragma once

#include "../device/device.h"
#include <stdint.h>

typedef uint8_t Filesystem_Entry_Type;
#define FILESYSTEM_ENTRY_FILE       ((Filesystem_Entry_Type)0)
#define FILESYSTEM_ENTRY_DIRECTORY  ((Filesystem_Entry_Type)1)

typedef uint16_t Filesystem_Entry_Attribute;
#define FILESYSTEM_ATTRIBUTE_READONLY   ((Filesystem_Entry_Attribute)0x01)
#define FILESYSTEM_ATTRIBUTE_EXECUTABLE ((Filesystem_Entry_Attribute)0x02)
#define FILESYSTEM_ATTRIBUTE_HIDDEN     ((Filesystem_Entry_Attribute)0x04)
#define FILESYSTEM_ATTRIBUTE_SYSTEM     ((Filesystem_Entry_Attribute)0x08)

typedef struct Filesystem_Inode
{
    uint64_t number;
    uint64_t size;
    uint64_t referenceCount;

    void* extra;

    Filesystem_Entry_Attribute attributes;
    Filesystem_Entry_Type type;
} Filesystem_Inode;

typedef struct Filesystem_Entry
{
    char name[512];
    uint64_t inode;
} Filesystem_Entry;

typedef struct Filesystem_File
{
    uint64_t pos;
    Filesystem_Inode* inode;

    void* extra;
} Filesystem_File;

#define FILESYSTEM_UNLINK_ERROR 0xFFFFFFFFFFFFFFFF

typedef struct Filesystem_Driver
{
    Block_Device* parent;

    void (*destroy)(struct Filesystem_Driver* driver);

    bool (*getRoot)(struct Filesystem_Driver* driver, Filesystem_Inode* out);

    // Inodes
    bool (*getInode)(struct Filesystem_Driver* driver, uint64_t number, Filesystem_Inode* out);
    bool (*removeInode)(struct Filesystem_Driver* driver, Filesystem_Inode* inode);

    // Directories
    uint64_t (*getEntryCount)(struct Filesystem_Driver* driver, Filesystem_Inode* dir);
    bool (*readEntry)(struct Filesystem_Driver* driver, Filesystem_File* dir, Filesystem_Entry* out);
    bool (*lookupEntry)(struct Filesystem_Driver* driver, Filesystem_Inode* dir, const char* name, Filesystem_Entry* out);

    bool (*createInode)(struct Filesystem_Driver* driver, Filesystem_Inode* dir, Filesystem_Entry_Type type,
                        Filesystem_Entry_Attribute attributes, const char* name, Filesystem_Inode* out);
    bool (*link)(struct Filesystem_Driver* driver, Filesystem_Inode* dir, const char* name, Filesystem_Inode* target);
    uint64_t (*unlink)(struct Filesystem_Driver* driver, Filesystem_Inode* dir, const char* name);

    // Files
    bool (*openFile)(struct Filesystem_Driver* driver, Filesystem_Inode* inode, Filesystem_File* out);
    void (*closeFile)(struct Filesystem_Driver* driver, Filesystem_File* file);
    uint64_t (*read)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, uint8_t* buffer);
    uint64_t (*write)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, const uint8_t* buffer);
    bool (*seek)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t pos);

    // General
    bool (*move)(struct Filesystem_Driver* driver, Filesystem_Inode* srcDir, const char* oldName, Filesystem_Inode* dstDir, const char* newName);

    void* data;
} Filesystem_Driver;
