#pragma once

#include <stdint.h>
#include <abi.h>
#include <stdbool.h>

void CDECL x86_invlpg(uint32_t addr);
void CDECL x86_reload_cr3();

bool x86_PageDirectory_Map(uintptr_t virtualAddr, uintptr_t physicalAddr);
void x86_PageDirectory_Unmap(uintptr_t virtualAddr);

