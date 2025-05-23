#include "paging.h"

#include <core/Defs.h>
#include <core/memory/memory.h>
#include <stddef.h>
#include "../stdio.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL invlpg(uint32_t addr);

uint64_t i686_get_paging_size(uint64_t size)
{
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t page_tables = (pages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    uint64_t total_size = PAGE_SIZE + page_tables * PAGE_SIZE;

    return total_size;
}

void* i686_paging_Initialize(PageDirectory* dir, uint64_t size, void* ptr)
{
    uint64_t paging_size = i686_get_paging_size(size);

    if (!ptr) {
        printf("No memory left for paging!");
        return NULL;
    }

    dir->directory = (uint32_t*)ptr;
    int num_page_tables = (int)((paging_size - PAGE_SIZE) / PAGE_SIZE);

    memset(dir->directory, 0, PAGE_SIZE);

    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (int pt = 0; pt < num_page_tables; pt++) {
        uint32_t* pt_entries = (uint32_t*)((uint8_t*)ptr + PAGE_SIZE + pt * PAGE_SIZE);
        memset(pt_entries, 0, PAGE_SIZE);

        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            int page_index = pt * PAGE_TABLE_ENTRIES + i;
            if (page_index >= pages) break;

            pt_entries[i] = (page_index * PAGE_SIZE) | (PAGE_PRESENT | PAGE_RW);
        }

        dir->directory[pt] = ((uint32_t)pt_entries) | (PAGE_PRESENT | PAGE_RW);
    }

    uintptr_t paging_base = (uintptr_t)ptr;
    uintptr_t paging_end = paging_base + paging_size;

    i686_paging_Load_Directory(dir->directory);

    return (void*)(paging_end);
}

void i686_paging_Load_Directory(uint32_t* page_directory)
{
    write_cr3((uint32_t)page_directory);
}

void i686_enable_paging()
{
    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000; // Setze das Paging-Bit (bit 31)
    write_cr0(cr0);
}