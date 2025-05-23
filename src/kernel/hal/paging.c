#include "paging.h"

#include "../debug.h"

PageDirectory getPageDirectory()
{
    PageDirectory dir;

    dir.directory_virtual = (uint32_t*)PAGE_DIRECTORY_PTR;
    dir.directory = (uint32_t*)i686_virt_to_phys((uint32_t*)dir.directory_virtual, (uintptr_t)dir.directory_virtual);

    return dir;
}

bool Paging_Map(uintptr_t virtual, uintptr_t physical)
{
    PageDirectory dir = getPageDirectory();

    if (!i686_map(dir.directory_virtual, virtual, physical))
        return false;
    return true;
}