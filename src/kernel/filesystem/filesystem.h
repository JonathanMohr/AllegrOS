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

typedef struct Filesystem_Node
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
    uint64_t node;
} Filesystem_Entry;

typedef struct Filesystem_File
{
    uint64_t pos;
    Filesystem_Node* node;

    void* extra;
} Filesystem_File;

#define FILESYSTEM_ENTRY_COUNT_ERROR 0xFFFFFFFFFFFFFFFF
#define FILESYSTEM_UNLINK_ERROR 0xFFFFFFFFFFFFFFFF

#define FILESYSTEM_DIR_ENTRY_FOUND 0
#define FILESYSTEM_DIR_END         1
#define FILESYSTEM_DIR_ERROR       2

typedef struct Filesystem_Driver
{
    Block_Device* parent;

    void (*destroy)(struct Filesystem_Driver* driver);

    bool (*getRoot)(struct Filesystem_Driver* driver, Filesystem_Node* out);

    // Inodes
    bool (*getNode)(struct Filesystem_Driver* driver, uint64_t number, Filesystem_Node* out);
    bool (*removeNode)(struct Filesystem_Driver* driver, Filesystem_Node* node);

    // Directories
    uint64_t (*getEntryCount)(struct Filesystem_Driver* driver, Filesystem_Node* dir);
    uint8_t (*readEntry)(struct Filesystem_Driver* driver, Filesystem_File* dir, Filesystem_Entry* out);
    uint8_t (*lookupEntry)(struct Filesystem_Driver* driver, Filesystem_Node* dir, const char* name, Filesystem_Entry* out);

    bool (*createInode)(struct Filesystem_Driver* driver, Filesystem_Node* dir, Filesystem_Entry_Type type,
                        Filesystem_Entry_Attribute attributes, const char* name, Filesystem_Node* out);
    bool (*link)(struct Filesystem_Driver* driver, Filesystem_Node* dir, const char* name, Filesystem_Node* target);
    uint64_t (*unlink)(struct Filesystem_Driver* driver, Filesystem_Node* dir, const char* name);

    // Files
    uint64_t (*read)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, uint8_t* buffer);
    uint64_t (*write)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t size, const uint8_t* buffer);
    bool (*seek)(struct Filesystem_Driver* driver, Filesystem_File* file, uint64_t pos);

    // General
    bool (*openFile)(struct Filesystem_Driver* driver, Filesystem_Node* inode, Filesystem_File* out);
    void (*closeFile)(struct Filesystem_Driver* driver, Filesystem_File* file);
    bool (*move)(struct Filesystem_Driver* driver, Filesystem_Node* srcDir, const char* oldName, Filesystem_Node* dstDir, const char* newName);

    void* data;

    bool caseSensitive;
} Filesystem_Driver;
