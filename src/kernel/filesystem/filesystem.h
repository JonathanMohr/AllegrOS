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

typedef struct Filesystem_Entry
{
    char name[512];

    uint64_t size;
    uint64_t pos;

    uint64_t handle;
    void* extra;

    Filesystem_Entry_Attribute attributes;

    Filesystem_Entry_Type type;
} Filesystem_Entry;

typedef struct Filesystem_Driver
{
    Block_Device* parent;

    void        (*flush)(struct Filesystem_Driver* driver);

    bool        (*openRoot)(struct Filesystem_Driver* driver, Filesystem_Entry* out);

    // Directories
    uint64_t    (*getEntryCount)(struct Filesystem_Driver* driver, Filesystem_Entry* parent);
    bool        (*openEntry)(struct Filesystem_Driver* driver, Filesystem_Entry* parent, uint64_t index, Filesystem_Entry* out);
    bool        (*createEntry)(struct Filesystem_Driver* driver, Filesystem_Entry* parent, Filesystem_Entry_Type type, Filesystem_Entry_Attribute attributes, const char* name, Filesystem_Entry* out);

    // Files
    uint64_t    (*read)(struct Filesystem_Driver* driver, Filesystem_Entry* entry, uint64_t size, void* buffer);
    uint64_t    (*write)(struct Filesystem_Driver* driver, Filesystem_Entry* entry, uint64_t size, const void* buffer);
    bool        (*seek)(struct Filesystem_Driver* driver, Filesystem_Entry* entry, uint64_t pos);

    // General
    bool        (*deleteEntry)(struct Filesystem_Driver* driver, Filesystem_Entry* entry);
    bool        (*moveEntry)(struct Filesystem_Driver* driver, Filesystem_Entry* entry, Filesystem_Entry* newParent, const char* newName);
    void        (*closeEntry)(struct Filesystem_Driver* driver, Filesystem_Entry* entry);

    void* data;
} Filesystem_Driver;
