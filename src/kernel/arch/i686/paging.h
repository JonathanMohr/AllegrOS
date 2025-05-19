#pragma once

#include <stdint.h>

#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

void i686_paging_Initialize();
void i686_paging_Load_Directory(uint32_t* page_directory);
void i686_enable_paging();