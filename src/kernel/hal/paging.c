#include "paging.h"

#include "../memory/memory.h"
#include "../debug.h"

PageDirectory g_PageDirectory;

PageDirectory* Paging_Initialize(uint64_t kernel_end)
{
    i686_paging_Initialize(&g_PageDirectory, kernel_end);

    i686_enable_paging();

    return &g_PageDirectory;
}

bool Paging_Map(PageDirectory* page_directory, uintptr_t virt_addr, uintptr_t phys_addr)
{
    int result = MAP_NOT_DONE;
    uint8_t tries = 0;
    while (result != MAP_SUCCESS)
    {
        result = i686_map_page(page_directory, virt_addr, phys_addr, PAGE_PRESENT | PAGE_RW, true);
        switch (result)
        {
            case MAP_MISSING_TABLE:
                if (i686_create_new_map(page_directory) < 0)
                    return false;
                break;

            case MAP_OUT_OF_RANGE:
                return false;
        }
        if (tries > 10)
            return false;
        tries++;
    }
    return true;
}

void Paging_Unmap(PageDirectory* page_directory, uintptr_t virt_addr)
{
    i686_unmap_page(page_directory, virt_addr);
}

void Paging_Load_Directory(PageDirectory* page_directory)
{
    i686_paging_Load_Directory((uint32_t*)page_directory);
}

PageDirectory* Paging_New_Directory()
{
    PageDirectory* page_dir = memory_Allocate(sizeof(PageDirectory), 1);

    page_dir->directory = memory_Allocate(PAGE_SIZE, 1);

    i686_initialize_page_directory(page_dir);
    
    return page_dir;
}