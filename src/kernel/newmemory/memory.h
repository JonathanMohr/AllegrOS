#pragma once

#include "result.h"
#include "arch.h"

#define MemoryLayout memoryLayout
extern Memory_Layout memoryLayout;

Memory_Result Memory_MapPageKernel(uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Memory_UnmapPageKernel(uintptr_t virtualAddr);
Memory_Result Memory_TranslateKernel(uintptr_t virtualAddr, uphysptr_t* out);
