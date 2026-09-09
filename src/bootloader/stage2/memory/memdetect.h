#pragma once

#include <bootparams.h>
#include "../x86/x86.h"

typedef struct {
    uint8_t* kernelAddress;

    uint8_t* pageDirectoryAddress;

    uint32_t pageTableCount;
    uint8_t* pageTableAddress;

    uint8_t* pageInfoAddress;
    uint64_t pageInfoSize;
} MemoryAddresses;

MemoryAddresses Memory_Detect(MemoryInfo* memoryInfo, x86_E820MemoryBlock* blocks, uint32_t count, uint32_t ksize);
