#include "kernel.h"
#include "memory.h"
#include "physical.h"
#include "../panic/panic.h"
#include "result.h"

#include <memory.h>
#include <stddef.h>
#include <stdbool.h>

uint8_t* bitmap = NULL;
uintptr_t bitmapPageCount = 0;

uintptr_t firstPageStart = 0;
uintptr_t usablePageCount = 0;

Memory_Result Memory_KernelVirtual_Initialize(MemoryInfo* memoryInfo)
{
    const uintptr_t kPageStart = ((uintptr_t)memoryInfo->startOfFree + memoryLayout.pageSize - 1) / memoryLayout.pageSize;
    const uintptr_t kPageEnd = memoryLayout.kernelSpaceEnd / memoryLayout.pageSize;
    const uintptr_t pages = kPageEnd - kPageStart;

    const uintptr_t bitsPerPage = memoryLayout.pageSize * 8;
    const uintptr_t bitmapPages = (pages + bitsPerPage) / (bitsPerPage + 1);

    bitmap = (uint8_t*)(kPageStart * memoryLayout.pageSize);
    bitmapPageCount = bitmapPages;

    firstPageStart = (kPageStart + bitmapPages) * memoryLayout.pageSize;
    usablePageCount = pages - bitmapPages;

    return MEMORY_SUCCESS;
}

static bool IsBitmapPageEmpty(uintptr_t bitmapPage)
{
    const uint8_t* page = (uint8_t*)bitmapPage;
    for (uintptr_t i = 0; i < memoryLayout.pageSize; i++)
    {
        if (page[i])
            return false;
    }
    return true;
}

Memory_Result Memory_KernelVirtual_AllocatePages(uint32_t pageCount, uintptr_t* out)
{
    if (pageCount == 0)
        return MEMORY_ERROR_NOT_ENOUGH_PAGES;

    uintptr_t runStart = 0;
    uintptr_t currentRun = 0;
    for (uintptr_t bitmapPage = 0; bitmapPage < bitmapPageCount; bitmapPage++)
    {
        const uintptr_t bitmapPageStartIndex = bitmapPage * memoryLayout.pageSize * 8;
        if (bitmapPageStartIndex >= usablePageCount)
            break;

        uphysptr_t tmp;
        Memory_Result translateResult = Memory_TranslateKernel((uintptr_t)bitmap + bitmapPage * memoryLayout.pageSize, &tmp);
        
        if (translateResult == MEMORY_SUCCESS)
        {
            for (uintptr_t i = 0; i < memoryLayout.pageSize * 8; i++)
            {
                const uintptr_t index = bitmapPageStartIndex + i;

                if (index >= usablePageCount)
                    break;

                uint8_t* entry = &bitmap[bitmapPage * memoryLayout.pageSize + i / 8];
                const uint8_t bit = (uint8_t)(1 << (i % 8));
                if ((*entry & bit) == 0)
                {
                    if (currentRun == 0)
                        runStart = bitmapPageStartIndex + i;
                    currentRun++;

                    if (currentRun >= pageCount)
                        break;
                }
                else
                    currentRun = 0;
            }
        }
        else if (translateResult == MEMORY_ERROR_NOT_MAPPED)
        {
            const uintptr_t available = usablePageCount - bitmapPageStartIndex;
            
            if (available == 0)
                break;

            if (currentRun == 0)
                runStart = bitmapPageStartIndex;

            currentRun += (available < memoryLayout.pageSize * 8) ? available : memoryLayout.pageSize * 8;
        }
        else
            return translateResult;

        if (currentRun >= pageCount)
        {
            const uintptr_t startPage = runStart * memoryLayout.pageSize + firstPageStart;
            const uintptr_t endBit = runStart + pageCount;

            const uintptr_t firstBitmapPage = runStart / (memoryLayout.pageSize * 8);
            const uintptr_t lastBitmapPage = (endBit - 1) / (memoryLayout.pageSize * 8);

            for (uintptr_t curBitmapPage = firstBitmapPage; curBitmapPage <= lastBitmapPage; curBitmapPage++)
            {
                const uintptr_t bitmapAddress = (uintptr_t)bitmap + curBitmapPage * memoryLayout.pageSize;

                uphysptr_t physicalPage;
                Memory_Result result = Memory_TranslateKernel(bitmapAddress, &physicalPage);

                if (result == MEMORY_ERROR_NOT_MAPPED)
                {
                    result = Memory_Physical_NewPage(&physicalPage);
                    if (result != MEMORY_SUCCESS)
                    {
                        for (uintptr_t bp = firstBitmapPage; bp < curBitmapPage; bp++)
                        {
                            // TODO: Optimize like in free
                            const uintptr_t address = (uintptr_t)bitmap + bp * memoryLayout.pageSize;
                            if (IsBitmapPageEmpty(address))
                            {
                                uphysptr_t phys;
                                if (Memory_TranslateKernel(address, &phys) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_TranslateKernel failed on %p\n", address);
                                    continue;
                                }

                                if (Memory_UnmapPageKernel(address) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_UnmapPageKernel failed on %p\n", address);
                                    continue;
                                }
                                
                                if (Memory_Physical_PutPage(phys) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_PutPage failed on %q\n", phys);
                                    continue;
                                }
                            }
                        }
                        return result;
                    }

                    result = Memory_MapPageKernel(bitmapAddress, physicalPage, MEMORY_READABLE | MEMORY_WRITABLE);
                    if (result != MEMORY_SUCCESS)
                    {
                        if (Memory_Physical_PutPage(physicalPage) != MEMORY_SUCCESS)
                        {
                            PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_PutPage failed on %q\n", physicalPage);
                        }
                        for (uintptr_t bp = firstBitmapPage; bp < curBitmapPage; bp++)
                        {
                            const uintptr_t address = (uintptr_t)bitmap + bp * memoryLayout.pageSize;
                            if (IsBitmapPageEmpty(address))
                            {
                                uphysptr_t phys;
                                if (Memory_TranslateKernel(address, &phys) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_TranslateKernel failed on %p\n", address);
                                    continue;
                                }

                                if (Memory_UnmapPageKernel(address) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_UnmapPageKernel failed on %p\n", address);
                                    continue;
                                }
                                
                                if (Memory_Physical_PutPage(phys) != MEMORY_SUCCESS)
                                {
                                    // Should not fail
                                    PanicMessageInfo("Memory_KernelVirtual_AllocatePages", "Memory_PutPage failed on %q\n", phys);
                                    continue;
                                }
                            }
                        }
                        return result;
                    }

                    memset((void*)bitmapAddress, 0, memoryLayout.pageSize);
                }
                else if (result != MEMORY_SUCCESS)
                    return result;
            }
            
            for (uintptr_t i = 0; i < pageCount; i++)
            {
                const uintptr_t bitIndex = runStart + i;
                bitmap[bitIndex / 8] |= (uint8_t)(1 << (bitIndex % 8));
            }

            *out = startPage;
            return MEMORY_SUCCESS;
        }
    }
    
    return MEMORY_ERROR_OUT_OF_MEMORY;
}

Memory_Result Memory_KernelVirtual_FreePages(uintptr_t address, uintptr_t pageCount)
{
    if (address % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    if (pageCount == 0)
        return MEMORY_ERROR_NOT_ENOUGH_PAGES;

    if (address < firstPageStart)
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    const uintptr_t startPage = (address - firstPageStart) / memoryLayout.pageSize;
    const uintptr_t endPage = startPage + pageCount;

    if (startPage >= usablePageCount || pageCount > usablePageCount - startPage)
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    const uintptr_t firstBitmapPage = startPage / (memoryLayout.pageSize * 8);
    const uintptr_t lastBitmapPage = (endPage - 1) / (memoryLayout.pageSize * 8);
    const uintptr_t startIndex = startPage % (memoryLayout.pageSize * 8);

    bool endedInBitmapPage = false;
    uintptr_t tmpStartIndex = startIndex;
    for (uintptr_t bitmapPage = firstBitmapPage; bitmapPage <= lastBitmapPage; bitmapPage++)
    {
        const uintptr_t bitmapPageStartIndex = bitmapPage * memoryLayout.pageSize * 8;

        const uintptr_t bitmapAddress = (uintptr_t)bitmap + bitmapPage * memoryLayout.pageSize;
        uphysptr_t tmp;
        Memory_Result translateResult = Memory_TranslateKernel(bitmapAddress, &tmp);
        if (translateResult != MEMORY_SUCCESS)
            return translateResult;

        for (uintptr_t i = tmpStartIndex; i < memoryLayout.pageSize * 8; i++)
        {
            const uintptr_t index = bitmapPageStartIndex + i;
            if (index >= endPage)
            {
                endedInBitmapPage = true;
                break;
            }

            uint8_t* entry = &bitmap[bitmapPage * memoryLayout.pageSize + i / 8];
            const uint8_t bit = (uint8_t)(1 << (i % 8));
            if ((*entry & bit) == 0)
                return MEMORY_ERROR_NOT_MAPPED;
        }
        tmpStartIndex = 0;
    }

    const bool oneFullPage = firstBitmapPage == lastBitmapPage && startIndex == 0 && !endedInBitmapPage;
    const bool firstPageIsFull = oneFullPage || (firstBitmapPage != lastBitmapPage && startIndex == 0);
    const bool lastPageIsFull = oneFullPage || (firstBitmapPage != lastBitmapPage && !endedInBitmapPage);

    tmpStartIndex = startIndex;
    for (uintptr_t bitmapPage = firstBitmapPage; bitmapPage <= lastBitmapPage; bitmapPage++)
    {
        const uintptr_t bitmapAddress = (uintptr_t)bitmap + bitmapPage * memoryLayout.pageSize;
        const uintptr_t bitmapPageStartIndex = bitmapPage * memoryLayout.pageSize * 8;

        for (uintptr_t i = tmpStartIndex; i < memoryLayout.pageSize * 8; i++)
        {
            const uintptr_t index = bitmapPageStartIndex + i;
            if (index >= endPage)
                break;

            uint8_t* entry = &bitmap[bitmapPage * memoryLayout.pageSize + i / 8];
            const uint8_t bit = (uint8_t)(1 << (i % 8));
            *entry &= ~bit;
        }
        tmpStartIndex = 0;

        if ((bitmapPage > firstBitmapPage && bitmapPage < lastBitmapPage) ||
            (firstPageIsFull && bitmapPage == firstBitmapPage) ||
            (lastPageIsFull && bitmapPage == lastBitmapPage) ||
            IsBitmapPageEmpty(bitmapAddress))
        {
            uphysptr_t physAddress;
            if (Memory_TranslateKernel(bitmapAddress, &physAddress) != MEMORY_SUCCESS)
            {
                // Should not fail
                PanicMessageInfo("Memory_KernelVirtual_FreePages", "Memory_TranslateKernel failed on %p\n", bitmapAddress);
                continue;
            }

            if (Memory_UnmapPageKernel(bitmapAddress) != MEMORY_SUCCESS)
            {
                // Should not fail
                PanicMessageInfo("Memory_KernelVirtual_FreePages", "Memory_UnmapPageKernel failed on %p\n", bitmapAddress);
                continue;
            }
                                
            if (Memory_Physical_PutPage(physAddress) != MEMORY_SUCCESS)
            {
                // Should not fail
                PanicMessageInfo("Memory_KernelVirtual_FreePages", "Memory_PutPage failed on %q\n", physAddress);
                continue;
            }
        }
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_KernelVirtual_IsAllocated(uintptr_t address, bool* out)
{
    if (address % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    if (address < firstPageStart)
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    const uintptr_t page = (address - firstPageStart) / memoryLayout.pageSize;

    if (page >= usablePageCount)
        return MEMORY_ERROR_OUT_OF_BOUNDS;

    const uintptr_t bitmapPage = page / (memoryLayout.pageSize * 8);
    const uintptr_t bitIndex = page % (memoryLayout.pageSize * 8);

    const uintptr_t bitmapAddress = (uintptr_t)bitmap + bitmapPage * memoryLayout.pageSize;

    uphysptr_t tmp;
    Memory_Result translateResult = Memory_TranslateKernel(bitmapAddress, &tmp);

    if (translateResult == MEMORY_ERROR_NOT_MAPPED)
    {
        *out = false;
        return MEMORY_SUCCESS;
    }
    else if (translateResult != MEMORY_SUCCESS)
        return translateResult;

    const uint8_t* entry = &bitmap[bitmapPage * memoryLayout.pageSize + bitIndex / 8];
    *out = (*entry & (1 << (bitIndex % 8))) != 0;

    return MEMORY_SUCCESS;
}
