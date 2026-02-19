#pragma once

#include <bootparams.h>
#include "../x86/x86.h"

void Memory_Detect(MemoryInfo* memoryInfo, x86_E820MemoryBlock* blocks, uint32_t count);
