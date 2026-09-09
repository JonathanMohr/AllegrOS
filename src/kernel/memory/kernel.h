#pragma once

#include <bootparams.h>
#include <stdbool.h>

#include "result.h"

Memory_Result Memory_KernelVirtual_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_KernelVirtual_AllocatePages(uint32_t pageCount, uintptr_t* out);
Memory_Result Memory_KernelVirtual_FreePages(uintptr_t address, uintptr_t pageCount);
Memory_Result Memory_KernelVirtual_IsAllocated(uintptr_t address, bool* out);
