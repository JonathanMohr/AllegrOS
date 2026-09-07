#pragma once

#include <abi.h>
#include <stdint.h>

void CDECL Task_SwitchTo(uintptr_t* oldStackPointer, uintptr_t newStackPointer);
void CDECL Task_JumpTo(uintptr_t newStackPointer);
uintptr_t CDECL Task_Create(uintptr_t stackTop, void* function, void* arg);
