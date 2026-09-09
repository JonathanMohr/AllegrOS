#include "memory.h"
#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

#include "../panic/panic.h"

#define MAX_SLAB_ALLOCATION_SIZE 1024

struct LargeEntry
{
    uintptr_t base;
    uintptr_t pages;
    struct LargeEntry* next;
};

static struct LargeEntry* large_head;

typedef struct
{
    struct LargeEntry* large_entry;
} large_data;


typedef struct SlabClass
{
    uint32_t objectSize;
    uintptr_t objectCount;
    uintptr_t bitmapBytes;
    uintptr_t slabHead;
} SlabClass;

static SlabClass slabClasses[] = {
    { 16, 0, 0, 0 }, { 32, 0, 0, 0 }, { 64, 0, 0, 0 }, { 128, 0, 0, 0 },
    { 256, 0, 0, 0 }, { 512, 0, 0, 0 }, { 1024, 0, 0, 0 }
};
#define SLAB_CLASS_COUNT (sizeof(slabClasses) / sizeof(slabClasses[0]))

typedef struct
{
    uintptr_t slabPage;
    uintptr_t classIndex;
    uintptr_t objectIndex;
} slab_data;

static void SlabClass_EnsureLayout(SlabClass* cls)
{
    if (cls->objectCount != 0)
        return;

    const uintptr_t footerSize = sizeof(uintptr_t) * 2;
    const uintptr_t available = memoryLayout.pageSize - footerSize;

    const uintptr_t groupSize = 8 * cls->objectSize + 1;
    const uintptr_t fullGroups = available / groupSize;
    const uintptr_t remaining = available - fullGroups * groupSize;

    uintptr_t extraObjects = 0;
    if (remaining >= 1)
        extraObjects = (remaining - 1) / cls->objectSize;

    cls->objectCount = fullGroups * 8 + extraObjects;
    cls->bitmapBytes = fullGroups + ((extraObjects > 0) ? 1 : 0);
}

static uint8_t* Slab_GetBitmap(const SlabClass* cls, uintptr_t slabPage)
{
    return (uint8_t*)(slabPage + cls->objectCount * cls->objectSize);
}

static void* Slab_GetObject(const SlabClass* cls, uintptr_t slabPage, uintptr_t index)
{
    return (void*)(slabPage + index * cls->objectSize);
}

static uintptr_t Slab_GetFreeCount(uintptr_t slabPage)
{
    uintptr_t freeCount;
    memcpy(&freeCount, (void*)(slabPage + memoryLayout.pageSize - sizeof(uintptr_t) - sizeof(uintptr_t)), sizeof(uintptr_t));
    return freeCount;
}

static void Slab_SetFreeCount(uintptr_t slabPage, uintptr_t freeCount)
{
    memcpy((void*)(slabPage + memoryLayout.pageSize - sizeof(uintptr_t) - sizeof(uintptr_t)), &freeCount, sizeof(uintptr_t));
}

static uintptr_t Slab_GetNext(uintptr_t slabPage)
{
    uintptr_t next;
    memcpy(&next, (void*)(slabPage + memoryLayout.pageSize - sizeof(uintptr_t)), sizeof(uintptr_t));
    return next;
}

static void Slab_SetNext(uintptr_t slabPage, uintptr_t next)
{
    memcpy((void*)(slabPage + memoryLayout.pageSize - sizeof(uintptr_t)), &next, sizeof(uintptr_t));
}

static int Slab_FindClassIndex(uintptr_t size)
{
    for (int i = 0; i < SLAB_CLASS_COUNT; i++)
    {
        if (size <= slabClasses[i].objectSize)
            return i;
    }
    return -1;
}


static void* large_alloc(uintptr_t size, int zero);
static bool large_free(void* ptr);
static bool large_find(void* ptr, uintptr_t* oldSize, large_data* data);
static void* large_realloc(void* ptr, uintptr_t size, bool zero, large_data* data);

static void* slab_alloc(uintptr_t size, bool zero);
static bool slab_free(void* ptr);
static bool slab_find(void* ptr, uintptr_t* oldSize, slab_data* data);
static void* slab_realloc(void* ptr, uintptr_t size, bool zero, slab_data* data);


static void* large_alloc(uintptr_t size, int zero)
{
    const uintptr_t pages = (size + memoryLayout.pageSize - 1) / memoryLayout.pageSize;
    
    uintptr_t base;
    Memory_Result result = Memory_Kernel_AllocateVirtual(pages, &base);
    if (result != MEMORY_SUCCESS)
        return NULL;

    result = Memory_LinkNew(MEMORY_KERNEL, base, MEMORY_WRITABLE, pages);

    if (result != MEMORY_SUCCESS)
    {
        if (Memory_Kernel_FreeVirtual(base, pages) != MEMORY_SUCCESS)
            PanicMessageInfo("large_alloc", "Memory_Kernel_FreeVirtual(%p, 0x%p) failed\n", base, pages);

        return NULL;
    }

    struct LargeEntry* entry = slab_alloc(sizeof(struct LargeEntry), 0);
    if (!entry)
    {
        if (Memory_Unlink(MEMORY_KERNEL, base, pages) != MEMORY_SUCCESS)
        {
            PanicMessageInfo("large_alloc", "Memory_Unlink(%p, %p, 0x%p) failed\n", MEMORY_KERNEL, base, pages);
            return NULL;
        }

        if (Memory_Kernel_FreeVirtual(base, pages) != MEMORY_SUCCESS)
            PanicMessageInfo("large_alloc", "Memory_Kernel_FreeVirtual(%p, 0x%p) failed\n", base, pages);

        return NULL;
    }

    entry->base = base;
    entry->pages = pages;
    entry->next = large_head;
    large_head = entry;

    if (zero)
        memset((void*)base, 0, size);

    return (void*)base;
}

static bool large_free(void* ptr)
{
    struct LargeEntry* before = NULL;
    struct LargeEntry* current = large_head;

    while (current)
    {
        if (current->base == (uintptr_t)ptr)
        {
            if (before)
                before->next = current->next;
            else
                large_head = current->next;

            if (Memory_Unlink(MEMORY_KERNEL, current->base, current->pages) != MEMORY_SUCCESS)
            {
                PanicMessageInfo("large_free", "Memory_Unlink(%p, %p, 0x%p) failed\n", MEMORY_KERNEL, current->base, current->pages);
                slab_free(current);
                return true;
            }

            if (Memory_Kernel_FreeVirtual( current->base, current->pages) != MEMORY_SUCCESS)
                PanicMessageInfo( "large_free", "Memory_Kernel_FreeVirtual(%p, 0x%p) failed\n", current->base, current->pages);

            slab_free(current);

            return true;
        }

        before = current;
        current = current->next;
    }

    return false;
}

static bool large_find(void* ptr, uintptr_t* oldSize, large_data* data)
{
    struct LargeEntry* current = large_head;

    while (current)
    {
        if (current->base == (uintptr_t)ptr)
        {
            data->large_entry = current;
            *oldSize = current->pages * memoryLayout.pageSize;
            return true;
        }

        current = current->next;
    }

    return false;
}

static void* large_realloc(void* ptr, uintptr_t size, bool zero, large_data* data)
{
    // TODO: Add later
    return NULL;
}


static void* slab_alloc(uintptr_t size, bool zero)
{
    int classIndex = Slab_FindClassIndex(size);
    if (classIndex < 0)
        return NULL;

    SlabClass* cls = &slabClasses[classIndex];
    SlabClass_EnsureLayout(cls);

    uintptr_t slabPage = cls->slabHead;
    while (slabPage)
    {
        if (Slab_GetFreeCount(slabPage) > 0)
            break;
        slabPage = Slab_GetNext(slabPage);
    }

    if (!slabPage)
    {
        uintptr_t virtualAddr;
        if (Memory_KernelVirtual_AllocatePages(1, &virtualAddr) != MEMORY_SUCCESS)
            return NULL;

        Memory_Result result = Memory_LinkNew(MEMORY_KERNEL, virtualAddr, MEMORY_WRITABLE, 1);
        if (result != MEMORY_SUCCESS)
        {
            if (Memory_Kernel_FreeVirtual(virtualAddr, 1) != MEMORY_SUCCESS)
                PanicMessageInfo("slab_alloc", "Memory_Kernel_FreeVirtual(%p, 1) failed\n", virtualAddr);
            return NULL;
        }

        memset(Slab_GetBitmap(cls, virtualAddr), 0, cls->bitmapBytes);
        Slab_SetFreeCount(virtualAddr, cls->objectCount);
        Slab_SetNext(virtualAddr, cls->slabHead);
        cls->slabHead = virtualAddr;

        slabPage = virtualAddr;
    }

    uint8_t* bitmap = Slab_GetBitmap(cls, slabPage);
    uint32_t index;
    for (index = 0; index < cls->objectCount; index++)
    {
        if ((bitmap[index / 8] & (1 << (index % 8))) == 0)
            break;
    }

    bitmap[index / 8] |= (uint8_t)(1 << (index % 8));
    Slab_SetFreeCount(slabPage, Slab_GetFreeCount(slabPage) - 1);

    void* obj = Slab_GetObject(cls, slabPage, index);
    if (zero)
        memset(obj, 0, cls->objectSize);

    return obj;
}

static bool slab_free(void* ptr)
{
    const uintptr_t addr = (uintptr_t)ptr;

    for (uintptr_t c = 0; c < SLAB_CLASS_COUNT; c++)
    {
        SlabClass* cls = &slabClasses[c];
        if (cls->objectCount == 0)
            continue;

        uintptr_t slabPage = cls->slabHead;
        uintptr_t before = 0;

        while (slabPage)
        {
            const uintptr_t objectsEnd = slabPage + cls->objectCount * cls->objectSize;
            if (addr >= slabPage && addr < objectsEnd && (addr - slabPage) % cls->objectSize == 0)
            {
                const uintptr_t index = (addr - slabPage) / cls->objectSize;

                uint8_t* bitmap = Slab_GetBitmap(cls, slabPage);
                if ((bitmap[index / 8] & (1 << (index % 8))) == 0)
                    return false;

                bitmap[index / 8] &= ~(uint8_t)(1 << (index % 8));

                uintptr_t freeCount = Slab_GetFreeCount(slabPage) + 1;
                Slab_SetFreeCount(slabPage, freeCount);

                if (freeCount == cls->objectCount)
                {
                    uintptr_t next = Slab_GetNext(slabPage);
                    if (before)
                        Slab_SetNext(before, next);
                    else
                        cls->slabHead = next;

                    if (Memory_Unlink(MEMORY_KERNEL, slabPage, 1) == MEMORY_SUCCESS)
                    {
                        if (Memory_Kernel_FreeVirtual(slabPage, 1) != MEMORY_SUCCESS)
                            PanicMessageInfo("slab_free", "Memory_Kernel_FreeVirtual(%p, 1) failed\n", slabPage);
                    }
                    else
                        PanicMessageInfo("slab_free", "Memory_Unlink(%p, %p, 1) failed\n", MEMORY_KERNEL, slabPage);
                }

                return true;
            }

            before = slabPage;
            slabPage = Slab_GetNext(slabPage);
        }
    }

    return false;
}

static bool slab_find(void* ptr, uintptr_t* oldSize, slab_data* data)
{
    const uintptr_t addr = (uintptr_t)ptr;

    for (uintptr_t c = 0; c < SLAB_CLASS_COUNT; c++)
    {
        SlabClass* cls = &slabClasses[c];
        if (cls->objectCount == 0)
            continue;

        uintptr_t slabPage = cls->slabHead;
        while (slabPage)
        {
            const uintptr_t objectsEnd = slabPage + cls->objectCount * cls->objectSize;
            if (addr >= slabPage && addr < objectsEnd && (addr - slabPage) % cls->objectSize == 0)
            {
                data->slabPage = slabPage;
                data->classIndex = c;
                data->objectIndex = (addr - slabPage) / cls->objectSize;
                *oldSize = cls->objectSize;
                return true;
            }

            slabPage = Slab_GetNext(slabPage);
        }
    }

    return false;
}

static void* slab_realloc(void* ptr, uintptr_t size, bool zero, slab_data* data)
{
    (void)zero;
    int newClassIndex = Slab_FindClassIndex(size);
    if (newClassIndex < 0 || (uintptr_t)newClassIndex != data->classIndex)
        return NULL;

    return ptr;
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
