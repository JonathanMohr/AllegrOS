#include "paging.h"

int i686_identity_map(PageDirectory* dir, uint32_t end_addr)
{

}

int i686_map_page(PageDirectory* dir, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags)
{
    uint32_t dir_index = (virt_addr >> 22) & 0x3FF;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    if (dir_index >= PAGE_DIRECTORY_ENTRIES)
        return MAP_OUT_OF_RANGE;

    uint32_t* page_table = dir->tables_virtual[dir_index];

    if (!dir->tables_virtual[dir_index]) {
        return MAP_MISSING_TABLE;
    }

    if (!dir->tables_virtual[dir_index])
    {
        return MAP_MISSING_TABLE;
    }

    page_table[table_index] = (phys_addr & 0xFFFFF000) | (flags & 0xFFF);

    // PD Eintrag setzen, falls nicht gesetzt
    if ((dir->directory[dir_index] & PAGE_PRESENT) == 0) {
        dir->directory[dir_index] = ((uintptr_t)page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
    }

    return MAP_SUCCESS;
}