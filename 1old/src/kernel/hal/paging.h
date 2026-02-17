#pragma once

#include "../arch/i686/paging.h"
#include <stdbool.h>

PageDirectory Paging_Initialize();
PageDirectory getPageDirectory();
bool Paging_Map(uintptr_t virtual, uintptr_t physical, bool user);

PageDirectory Paging_Create(PageDirectory* kernel);
void Paging_Load(PageDirectory* pageDirectory);