#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

uint64_t i686_get_paging_size(uint64_t size);
bool i686_paging_Initialize(uint64_t kernel_end);
void i686_paging_Load_Directory(uint32_t* page_directory);
void i686_enable_paging();
bool i686_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags);
void i686_unmap_page(uint32_t virtual_addr);