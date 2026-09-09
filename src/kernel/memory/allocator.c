#include "memory.h"
#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

#include "../panic/panic.h"

#define MAX_SLAB_ALLOCATION_SIZE 1024

typedef struct
{
    // TODO
} large_data;

typedef struct
{
    // TODO
} slab_data;


static void* large_alloc(uintptr_t size, int zero)
{
    // TODO
}

static bool large_free(void* ptr)
{
    // TODO
}

static bool large_find(void* ptr, uintptr_t* oldSize, large_data* data)
{
    // TODO
}

static void* large_realloc(void* ptr, uintptr_t size, bool zero, large_data* data)
{
    // TODO
}


static void* slab_alloc(uintptr_t size, bool zero)
{
    // TODO
}

static bool slab_free(void* ptr)
{
    // TODO
}

static bool slab_find(void* ptr, uintptr_t* oldSize, slab_data* data)
{
    // TODO
}

static void* slab_realloc(void* ptr, uintptr_t size, bool zero, slab_data* data)
{
    // TODO
}


static void* allocate(uintptr_t size, bool zero)
{
    if (size == 0)
        return NULL;
    if (size <= MAX_SLAB_ALLOCATION_SIZE)
        return slab_alloc(size, zero);
    return large_alloc(size, zero);
}

static void* reallocate(void* ptr, uintptr_t newSize, bool zero)
{
    union
    {
        slab_data slab;
        large_data large;
    } data;
    uintptr_t oldSize;
    void* newPtr;
    
    if (!ptr)
        return allocate(newSize, zero);

    if (slab_find(ptr, &oldSize, &data.slab))
    {
        if (newSize <= MAX_SLAB_ALLOCATION_SIZE)
        {
            newPtr = slab_realloc(ptr, newSize, zero, &data.slab);
            if (newPtr)
                return newPtr;
        }

        newPtr = allocate(newSize, zero);
        if (!newPtr)
            return NULL;

        memcpy(newPtr, ptr, (oldSize < newSize) ? oldSize : newSize);

        (void)slab_free(ptr);

        return newPtr;
    }

    if (large_find(ptr, &oldSize, &data.large))
    {
        if (newSize > MAX_SLAB_ALLOCATION_SIZE)
        {
            newPtr = large_realloc(ptr, newSize, zero, &data.large);
            if (newPtr)
                return newPtr;
        }

        newPtr = allocate(newSize, zero);
        if (!newPtr)
            return NULL;

        memcpy(newPtr, ptr, (oldSize < newSize) ? oldSize : newSize);

        (void)large_free(ptr);

        return newPtr;
    }

    return NULL;
}

void* Memory_KernelAllocate(uintptr_t size)
{
    return allocate(size, false);
}

void* Memory_KernelZallocate(uintptr_t size)
{
    return allocate(size, true);
}

void* Memory_KernelReallocate(void* ptr, uintptr_t newSize)
{
    return reallocate(ptr, newSize, false);
}

void* Memory_KernelRezallocate(void* ptr, uintptr_t newSize)
{
    return reallocate(ptr, newSize, true);
}

void Memory_KernelFree(void* ptr)
{
    if (!ptr)
        return;
    if (slab_free(ptr))
        return;
    if (large_free(ptr))
        return;
    PanicMessageInfo("Memory_KernelFree", "Was called with not allocated pointer %p\n", (uintptr_t)ptr);
}
