#include "paging.h"

#define VGA_TEXT_BUFFER_START 0xB8000
#define VGA_TEXT_BUFFER_SIZE 4096 // 4 KB

#include "../debug.h"
#include "../memory/memory.h"
#include <stddef.h>
#include "vfs.h"

PageDirectory Paging_Initialize()
{
    uint8_t* VGA_addr = memory_virtualAllocate(VGA_TEXT_BUFFER_SIZE, VGA_TEXT_BUFFER_SIZE);
    if (!VGA_addr)
    {
        PageDirectory empty_pd = {0};
        return empty_pd;
    }

    if (!Paging_Map((uintptr_t)VGA_addr, VGA_TEXT_BUFFER_START))
    {
        memory_virtualFree(VGA_addr);
        PageDirectory empty_pd = {0};
        return empty_pd;
    }

    VFS_Initialize(VGA_addr);

    return getPageDirectory();
}

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