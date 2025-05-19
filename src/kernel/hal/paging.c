#include "paging.h"

#include "../arch/i686/paging.h"

void Paging_Initialize()
{
    i686_paging_Initialize();

    i686_enable_paging();
}