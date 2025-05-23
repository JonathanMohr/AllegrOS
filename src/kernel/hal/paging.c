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

PageDirectory Paging_Create(PageDirectory* kernel)
{
    PageDirectory pageDirectory = i686_create_page_directory(kernel->directory_virtual);
    return pageDirectory;
}

void Paging_Load(PageDirectory* pageDirectory)
{
    i686_load_page_directory(pageDirectory);
}