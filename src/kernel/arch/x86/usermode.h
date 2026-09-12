#pragma once

#include <abi.h>
#include <stdint.h>

void CDECL Arch_JumpToUserMode(uintptr_t entryPoint, uintptr_t stackTop);
