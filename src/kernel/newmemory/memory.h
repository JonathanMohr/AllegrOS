#pragma once

#include "result.h"
#include <stdint.h>

typedef struct
{
    uintptr_t kernelSpaceStart;
    uintptr_t kernelSpaceEnd;

    uintptr_t userSpaceStart;
    uintptr_t userSpaceEnd;

    size_t pageSize;
} Memory_Layout;

extern Memory_Layout MemoryLayout;
