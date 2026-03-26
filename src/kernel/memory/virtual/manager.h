#pragma once

#include <bootparams.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Memory_VirtualAllocator {
    uintptr_t base;
    uintptr_t size;
    uintptr_t used;

    bool initialized;
} Memory_VirtualAllocator;

// TODO
