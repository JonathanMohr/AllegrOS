#pragma once

#include <stdint.h>
#include <stdbool.h>

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

uint64_t i686_get_paging_size(uint64_t size);
void i686_initialize_page_directory(PageDirectory* page_directory);
bool i686_paging_Initialize(PageDirectory* dir, uint64_t kernel_end);
void i686_paging_Load_Directory(uint32_t* page_directory);
void i686_enable_paging();
int i686_map_page(PageDirectory* dir, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags, bool safeguard);
void i686_unmap_page(PageDirectory* dir, uint32_t virtual_addr);
int i686_create_new_map(PageDirectory* dir);