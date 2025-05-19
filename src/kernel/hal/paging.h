#pragma once

#include <stdint.h>
#include <stdbool.h>

void Paging_Initialize(uint64_t kernel_end);
bool Paging_Map(uintptr_t virt_addr, uintptr_t phys_addr);
void Paging_Unmap(uintptr_t virt_addr);