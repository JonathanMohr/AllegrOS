#pragma once

#include <stdint.h>

typedef uint8_t Memory_Result;
#define MEMORY_SUCCESS 0
#define MEMORY_ERROR_INTERNAL 1
#define MEMORY_ERROR_OUT_OF_MEMORY 2
#define MEMORY_ERROR_OUT_OF_BOUNDS 3
#define MEMORY_ERROR_REFERENCE_LIMIT 4
#define MEMORY_ERROR_NOT_REFERENCED 5
#define MEMORY_ERROR_NOT_MAPPED 6
#define MEMORY_ERROR_ALREADY_MAPPED 7
#define MEMORY_ERROR_PAGE_SIZE_TOO_SMALL 8
#define MEMORY_ERROR_NOT_ENOUGH_PAGES 9
#define MEMORY_ERROR_NOT_ALIGNED 10

typedef uint8_t Memory_Flags;
#define MEMORY_WRITABLE 0x1
#define MEMORY_USER     0x2
#define MEMORY_GLOBAL   0x4

typedef struct
{
    uintptr_t kernelSpaceStart;
    uintptr_t kernelSpaceEnd;

    uintptr_t userSpaceStart;
    uintptr_t userSpaceEnd;

    size_t pageSize;
} Memory_Layout;
