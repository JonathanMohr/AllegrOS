#include "paging.h"

#include "../arch/i686/paging.h"
#include "../memory/memory.h"
#include "../debug.h"

void Paging_Initialize(uint64_t kernel_end)
{
    i686_paging_Initialize(kernel_end);

    i686_enable_paging();
}