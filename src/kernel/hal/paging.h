#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../arch/i686/paging.h"

PageDirectory* Paging_Initialize(uint64_t kernel_end);
bool Paging_Map(PageDirectory* page_directory, uintptr_t virt_addr, uintptr_t phys_addr);
void Paging_Unmap(PageDirectory* page_directory, uintptr_t virt_addr);
void Paging_Load_Directory(PageDirectory* page_directory);
PageDirectory* Paging_New_Directory();