#include "kernel.h"
#include "memory.h"
#include "physical.h"

#include <memory.h>
#include <stddef.h>

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
                const uint8_t bit = 1 << (i % 8);
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

            for (uintptr_t bitmapPage = firstBitmapPage; bitmapPage <= lastBitmapPage; bitmapPage++)
            {
                const uintptr_t bitmapAddress = (uintptr_t)bitmap + bitmapPage * memoryLayout.pageSize;

                uphysptr_t physicalPage;
                Memory_Result result = Memory_TranslateKernel(bitmapAddress, &physicalPage);

                if (result == MEMORY_ERROR_NOT_MAPPED)
                {
                    result = Memory_Physical_NewPage(&physicalPage);
                    if (result != MEMORY_SUCCESS)
                        return result;

                    result = Memory_MapPageKernel(bitmapAddress, physicalPage, MEMORY_WRITABLE);
                    if (result != MEMORY_SUCCESS)
                    {
                        Memory_Physical_PutPage(physicalPage);
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

Memory_Result Memory_KernelVirtual_FreePage(uintptr_t page, uintptr_t pageCount)
{
    // TODO
}
