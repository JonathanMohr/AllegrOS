#pragma once

#include <stdint.h>
#include <abi.h>
#include <stdbool.h>

void CDECL x86_invlpg(uintptr_t addr);
void CDECL x86_reload_cr3();
void CDECL x86_load_cr3(uint32_t pageDirectory);
