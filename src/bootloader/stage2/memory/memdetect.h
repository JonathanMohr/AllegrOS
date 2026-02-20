#pragma once

#include <bootparams.h>
#include "../x86/x86.h"

typedef struct {
    uint8_t* kernelAddress;
    uint8_t* kernelPageTableAddress;
} MemoryAddresses;

MemoryAddresses Memory_Detect(MemoryInfo* memoryInfo, x86_E820MemoryBlock* blocks, uint32_t count, uint32_t kernel_size);
