#include "physical.h"
#include "memory.h"
#include "arch.h"
#include "result.h"
#include <stddef.h>
#include <memory.h>
#include <stdbool.h>

struct FreePage
{
    uphysptr_t next;
};
static uphysptr_t freeListHead = 0;

static uint32_t* pageReferences = NULL;
static uint64_t pageCount = 0;

#define REFERENCES_NOT_COUNTED 0xFFFFFFFF
#define MAX_REFERENCES 0xFFFFFFFE

static void FreePage(uphysptr_t physicalAddress)
{
    struct FreePage* temp = (struct FreePage*)Arch_TemporaryMap(physicalAddress);
    temp->next = freeListHead;
    freeListHead = physicalAddress;
    Arch_TemporaryUnmap((uintptr_t)temp);
}

static bool GetFreePage(uphysptr_t* out)
{
    if (!freeListHead)
        return false;

    *out = freeListHead;
    struct FreePage* temp = (struct FreePage*)Arch_TemporaryMap(freeListHead);
    freeListHead = temp->next;
    Arch_TemporaryUnmap((uintptr_t)temp);

    return true;
}

static bool GetPageReferences(uphysptr_t physicalAddress, uint32_t** out)
{
    uint64_t pageIndex = physicalAddress / MemoryLayout.pageSize;
    if (pageIndex >= pageCount)
        return false;

    *out = &pageReferences[pageIndex];
    return true;
}


Memory_Result Memory_Physical_Initialize(MemoryInfo* memoryInfo)
{
    uint64_t totalPages = 0;
    uphysptr_t highestAddress = 0;

    // TODO: set references for not usable pages to REFERENCES_NOT_COUNTED

    for (uint32_t i = 0; i < memoryInfo->RegionCount; i++)
    {
        MemoryRegion* region = &memoryInfo->Regions[i];
        if (region->Type != MEMORY_TYPE_USABLE)
            continue;

        uphysptr_t start = ((region->Begin + MemoryLayout.pageSize - 1) / MemoryLayout.pageSize) * MemoryLayout.pageSize;
        uphysptr_t end = ((region->Begin + region->Length) / MemoryLayout.pageSize) * MemoryLayout.pageSize;

        for (uphysptr_t addr = start; addr < end; addr += MemoryLayout.pageSize)
        {
            FreePage(addr);
            totalPages++;
        }

        if (end > highestAddress)
            highestAddress = end;
    }

    if (!totalPages)
        return MEMORY_ERROR_OUT_OF_MEMORY;

    pageCount = (highestAddress + MemoryLayout.pageSize - 1) / MemoryLayout.pageSize;
    pageReferences = memoryInfo->physPageArray;
    memset(pageReferences, 0, pageCount * sizeof(uint32_t));

    return MEMORY_SUCCESS;
}


Memory_Result Memory_Physical_NewPage(uphysptr_t* out)
{
    uphysptr_t phys;
    if (!GetFreePage(&phys))
        return MEMORY_ERROR_OUT_OF_MEMORY;

    uint32_t* references;
    if (!GetPageReferences(phys, &references))
    {
        FreePage(phys);
        return MEMORY_ERROR_INTERNAL;
    }

    *references = 1;
    *out = phys;

    return MEMORY_SUCCESS;
}

Memory_Result Memory_Physical_GetPage(uphysptr_t page)
{
    uint32_t* references;
    if (!GetPageReferences(page, &references))
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    if (*references >= MAX_REFERENCES)
        return MEMORY_ERROR_REFERENCE_LIMIT;

    (*references)++;

    return MEMORY_SUCCESS;
}

Memory_Result Memory_Physical_PutPage(uphysptr_t page)
{
    uint32_t* references;
    if (!GetPageReferences(page, &references))
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    if (*references == 0)
        return MEMORY_ERROR_NOT_REFERENCED;

    if (--(*references) == 0)
        FreePage(page);

    return MEMORY_SUCCESS;
}
