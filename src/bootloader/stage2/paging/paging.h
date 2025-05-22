#pragma once

#include <stdint.h>

#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

typedef enum {
    MAP_SUCCESS = 1,
    MAP_NOT_DONE = 0,
    MAP_OUT_OF_RANGE = -1,
    MAP_MISSING_TABLE = -2
} MapResult;

typedef struct {
    uint32_t* directory;            // Physikalisch
    uint32_t* tables_virtual[1024]; // Nur zum Zugriff im Kernel
} PageDirectory;

int i686_map_page(PageDirectory* dir, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags);