#pragma once

#include <boot/arch/i686/paging.h>

#define PAGE_DIRECTORY_PTR 0xFFFFF000

uintptr_t i686_virt_to_phys(uint32_t* pageDirectory, uintptr_t virtual_addr);