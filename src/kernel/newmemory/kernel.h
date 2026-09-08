#pragma once

#include <bootparams.h>

#include "result.h"

Memory_Result Memory_KernelVirtual_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_KernelVirtual_AllocatePages(uint32_t pageCount, uintptr_t* out);
Memory_Result Memory_KernelVirtual_FreePage(uintptr_t page);
