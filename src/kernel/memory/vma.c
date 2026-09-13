#include "kernel.h"
#include "memory.h"
#include "result.h"

Memory_Result Memory_AllocateVirtual(AddressSpace* addressSpace, uintptr_t pageCount, Memory_Flags flags, uintptr_t* out)
{
    if (addressSpace == MEMORY_KERNEL)
        return Memory_KernelVirtual_AllocatePages(pageCount, out);

    const uintptr_t userSpaceEnd = memoryLayout.userSpaceEnd / MEMORY_PAGE_SIZE;

    uintptr_t lastEnd = (memoryLayout.userSpaceStart + MEMORY_PAGE_SIZE - 1) / MEMORY_PAGE_SIZE;
    Virtual_Memory_Area* before = NULL;
    Virtual_Memory_Area* vma = addressSpace->VMAs;

    while (1)
    {
        uintptr_t freeCount = (vma ? vma->start : userSpaceEnd) - lastEnd;

        if (freeCount >= pageCount)
        {
            Virtual_Memory_Area* newVMA = Memory_KernelAllocate(sizeof(Virtual_Memory_Area));
            if (!newVMA)
                return MEMORY_ERROR_OUT_OF_MEMORY;
            newVMA->next = vma;
            newVMA->start = lastEnd;
            newVMA->pageCount = pageCount;
            newVMA->flags = flags;

            if (before)
                before->next = newVMA;
            else
                addressSpace->VMAs = newVMA;

            *out = lastEnd * MEMORY_PAGE_SIZE;
            return MEMORY_SUCCESS;
        }

        if (!vma)
            break;
        lastEnd = vma->start + vma->pageCount;
        before = vma;
        vma = vma->next;
    }

    return MEMORY_ERROR_OUT_OF_MEMORY;
}

Memory_Result Memory_FreeVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount, bool strict)
{
    if (virtualAddress % MEMORY_PAGE_SIZE != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    if (addressSpace == MEMORY_KERNEL)
        return Memory_KernelVirtual_FreePages(virtualAddress, pageCount);

    const uintptr_t freeStart = virtualAddress / MEMORY_PAGE_SIZE;
    const uintptr_t freeEnd = freeStart + pageCount;

    if (strict)
    {
        uintptr_t covered = freeStart;
        Virtual_Memory_Area* checkVMA = addressSpace->VMAs;

        while (checkVMA && covered < freeEnd)
        {
            const uintptr_t vmaStart = checkVMA->start;
            const uintptr_t vmaEnd = checkVMA->start + checkVMA->pageCount;

            if (vmaEnd <= covered)
            {
                checkVMA = checkVMA->next;
                continue;
            }

            if (vmaStart > covered)
                return MEMORY_ERROR_NOT_MAPPED;

            covered = vmaEnd;
            checkVMA = checkVMA->next;
        }

        if (covered < freeEnd)
            return MEMORY_ERROR_NOT_MAPPED;
    }

    Virtual_Memory_Area* before = NULL;
    Virtual_Memory_Area* vma = addressSpace->VMAs;

    Virtual_Memory_Area* backup = Memory_KernelAllocate(sizeof(Virtual_Memory_Area));
    if (!backup)
        return MEMORY_ERROR_OUT_OF_MEMORY;
    Virtual_Memory_Area* backup2 = Memory_KernelAllocate(sizeof(Virtual_Memory_Area));
    if (!backup2)
    {
        Memory_KernelFree(backup);
        return MEMORY_ERROR_OUT_OF_MEMORY;
    }

    while (vma)
    {
        const uintptr_t vmaStart = vma->start;
        const uintptr_t vmaEnd = vma->start + vma->pageCount;

        if (vmaEnd <= freeStart || vmaStart >= freeEnd)
        {
            before = vma;
            vma = vma->next;
            continue;
        }

        if (freeStart <= vmaStart && freeEnd >= vmaEnd)
        {
            Virtual_Memory_Area* toRemove = vma;
            vma = vma->next;

            if (before)
                before->next = vma;
            else
                addressSpace->VMAs = vma;

            Memory_KernelFree(toRemove);
            continue;
        }

        if (freeStart <= vmaStart)
        {
            vma->pageCount = vmaEnd - freeEnd;
            vma->start = freeEnd;

            before = vma;
            vma = vma->next;
            continue;
        }

        if (freeEnd >= vmaEnd)
        {
            vma->pageCount = freeStart - vmaStart;

            before = vma;
            vma = vma->next;
            continue;
        }

        Virtual_Memory_Area* newVMA = backup;
        backup = backup2;
        backup2 = NULL;

        newVMA->start = freeEnd;
        newVMA->pageCount = vmaEnd - freeEnd;
        newVMA->flags = vma->flags;
        newVMA->next = vma->next;

        vma->pageCount = freeStart - vmaStart;
        vma->next = newVMA;

        before = newVMA;
        vma = newVMA->next;
    }

    if (backup)
        Memory_KernelFree(backup);
    if (backup2)
        Memory_KernelFree(backup2);

    return MEMORY_SUCCESS;
}

Memory_Result Memory_ReserveVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount, Memory_Flags flags)
{
    if (virtualAddress % MEMORY_PAGE_SIZE != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    if (addressSpace == MEMORY_KERNEL)
        return MEMORY_ERROR_INTERNAL; // TODO

    const uintptr_t reserveStart = virtualAddress / MEMORY_PAGE_SIZE;
    const uintptr_t reserveEnd = reserveStart + pageCount;

    const uintptr_t userSpaceStartPage = (memoryLayout.userSpaceStart + MEMORY_PAGE_SIZE - 1) / MEMORY_PAGE_SIZE;
    const uintptr_t userSpaceEndPage = memoryLayout.userSpaceEnd / MEMORY_PAGE_SIZE;

    if (reserveStart < userSpaceStartPage || reserveEnd > userSpaceEndPage)
        return MEMORY_ERROR_DOMAIN;

    Virtual_Memory_Area* before = NULL;
    Virtual_Memory_Area* vma = addressSpace->VMAs;

    while (vma && vma->start + vma->pageCount <= reserveStart)
    {
        before = vma;
        vma = vma->next;
    }

    if (vma && vma->start < reserveEnd)
        return MEMORY_ERROR_ALREADY_MAPPED;

    Virtual_Memory_Area* newVMA = Memory_KernelAllocate(sizeof(Virtual_Memory_Area));
    if (!newVMA)
        return MEMORY_ERROR_OUT_OF_MEMORY;

    newVMA->start = reserveStart;
    newVMA->pageCount = pageCount;
    newVMA->flags = flags;
    newVMA->next = vma;

    if (before)
        before->next = newVMA;
    else
        addressSpace->VMAs = newVMA;

    return MEMORY_SUCCESS;
}

Memory_Result Memory_GetVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t* outStart, uintptr_t* outSize, Memory_Flags* outFlags)
{
    if (addressSpace == MEMORY_KERNEL)
        return MEMORY_ERROR_DOMAIN;

    const uintptr_t addressPage = virtualAddress / MEMORY_PAGE_SIZE;

    Virtual_Memory_Area* vma = addressSpace->VMAs;

    while (vma && vma->start + vma->pageCount <= addressPage)
        vma = vma->next;

    if (!vma || vma->start > addressPage)
        return MEMORY_ERROR_NOT_MAPPED;

    *outStart = vma->start * MEMORY_PAGE_SIZE;
    *outSize = vma->pageCount * MEMORY_PAGE_SIZE;
    *outFlags = vma->flags;

    return MEMORY_SUCCESS;
}
