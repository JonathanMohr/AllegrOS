#pragma once

#include <boot/arch/i686/paging.h>
#include <stdbool.h>

#define PAGE_DIRECTORY_PTR 0xFFFFF000

uintptr_t i686_virt_to_phys(uint32_t* pageDirectory, uintptr_t virtual_addr);

bool i686_map(uint32_t* pageDirectory, uintptr_t virtual_addr, uintptr_t physical_addr);