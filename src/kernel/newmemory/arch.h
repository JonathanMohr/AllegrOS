#pragma once

#include "result.h"
#include <stdint.h>

void Arch_Initialize(Memory_Layout* layout);

uintptr_t Arch_TemporaryMap(uphysptr_t physicalMapAddress);
void Arch_TemporaryUnmap(uintptr_t virtualMapAddress);

Memory_Result Arch_CreateAddressSpace(uphysptr_t* out);
void Arch_DestroyAddressSpace(uphysptr_t addressSpace);

Memory_Result Arch_MapPage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Arch_UnmapPage(uphysptr_t addressSpace, uintptr_t virtualAddr);
Memory_Result Arch_TranslatePage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t* outPhysicalAddr);

void Arch_SwitchAddressSpace(uphysptr_t addressSpace);
