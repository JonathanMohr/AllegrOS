#pragma once

#include "result.h"
#include <stdint.h>
#include <bootparams.h>

void Arch_Memory_Initialize(Memory_Layout* layout, MemoryInfo* memoryInfo);

uintptr_t Arch_TemporaryMap(uphysptr_t physicalMapAddress);
void Arch_TemporaryUnmap(uintptr_t virtualMapAddress);

Memory_Result Arch_CreateAddressSpace(uphysptr_t* out);
void Arch_DestroyAddressSpace(uphysptr_t addressSpace);

Memory_Result Arch_MapPage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Arch_UnmapPage(uphysptr_t addressSpace, uintptr_t virtualAddr);
Memory_Result Arch_TranslatePage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t* outPhysicalAddr);
Memory_Result Arch_ChangeFlags(uphysptr_t addressSpace, uintptr_t virtualAddr, Memory_Flags flags);
Memory_Result Arch_GetFlags(uphysptr_t addressSpace, uintptr_t virtualAddr, Memory_Flags* out);

Memory_Result Arch_SyncKernel(uphysptr_t addressSpace, uphysptr_t newAddressSpace);

void Arch_SwitchAddressSpace(uphysptr_t addressSpace);
