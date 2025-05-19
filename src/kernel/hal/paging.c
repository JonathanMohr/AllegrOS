#include "paging.h"

#include "../arch/i686/paging.h"
#include "../memory/memory.h"
#include "../debug.h"

void Paging_Initialize(uint64_t kernel_end)
{
    i686_paging_Initialize(kernel_end);

    i686_enable_paging();
}

bool Paging_Map(uintptr_t virt_addr, uintptr_t phys_addr)
{
    return i686_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_RW);
}

void Paging_Unmap(uintptr_t virt_addr)
{
    i686_unmap_page(virt_addr);
}