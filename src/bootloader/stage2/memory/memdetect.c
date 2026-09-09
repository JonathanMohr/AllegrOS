#include "memdetect.h"

#include <bootparams.h>
#include <stddef.h>
#include <minmax.h>
#include "../x86/x86.h"

MemoryAddresses Memory_Detect(MemoryInfo* memoryInfo, x86_E820MemoryBlock* blocks, uint32_t count, uint32_t ksize)
{
    for (uint32_t i = 0; i < count - 1; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (blocks[j].Base < blocks[i].Base)
            {
                x86_E820MemoryBlock tmp = blocks[i];
                blocks[i] = blocks[j];
                blocks[j] = tmp;
            }
        }
    }

    uint64_t highestAddress = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        uint64_t end = blocks[i].Base + blocks[i].Length;
        if (end > highestAddress)
            highestAddress = end;
    }

    uint32_t regionCount = 0;
    uint64_t lastEnd = 0;

    uint32_t kernel_size = (ksize + 0xFFF) & ~0xFFF;

    uint64_t totalPageCount = (highestAddress + 0xFFF) / 0x1000;
    uint64_t pageInfo_size = (totalPageCount * sizeof(uint32_t) + 0xFFF) & ~0xFFF;

    uint32_t kernel_pageTableCount = (kernel_size + pageInfo_size + 0x3FFFFF) / 0x400000;
    uint32_t kernel_pageTableSize = kernel_pageTableCount * 0x1000;

    uint8_t* page_directory = NULL;
    uint8_t* page_tables = NULL;
    uint8_t* kernel_address = NULL;
    uint8_t* pageInfo_address = NULL;

    for (uint32_t i = 0; i < count; i++)
    {
        uint64_t base = blocks[i].Base;
        uint64_t length = blocks[i].Length;
        uint32_t type = blocks[i].Type;
        uint32_t acpi = blocks[i].ACPI;

        // Gap
        if (base > lastEnd)
        {
            memoryInfo->Regions[regionCount].Begin = lastEnd;
            memoryInfo->Regions[regionCount].Length = base - lastEnd;
            memoryInfo->Regions[regionCount].Type = MEMORY_TYPE_HARDWARE;
            memoryInfo->Regions[regionCount].ACPI = 0;
            regionCount++;
        }

        // Overlap
        if (base < lastEnd)
        {
            uint64_t overlap = lastEnd - base;
            base += overlap;
            if (length <= overlap) continue;
            length -= overlap;
        }

        if (base < 0xFFFFF && type == MEMORY_TYPE_USABLE)
            type = MEMORY_TYPE_RELUCTANT;

        // Kernel
        if (!kernel_address && length >= kernel_size && type == MEMORY_TYPE_USABLE)
        {
            memoryInfo->Regions[regionCount].Begin = base;
            memoryInfo->Regions[regionCount].Length = kernel_size;
            memoryInfo->Regions[regionCount].Type = MEMORY_TYPE_KERNEL;
            memoryInfo->Regions[regionCount].ACPI = 0;
            regionCount++;

            kernel_address = (uint8_t*)(uintptr_t)base;

            base += kernel_size;
            if (length == kernel_size) continue;
            length -= kernel_size;
        }

        // Page Tables
        if (!page_tables && length >= kernel_pageTableSize && type == MEMORY_TYPE_USABLE)
        {
            memoryInfo->Regions[regionCount].Begin = base;
            memoryInfo->Regions[regionCount].Length = kernel_pageTableSize;
            memoryInfo->Regions[regionCount].Type = MEMORY_TYPE_KERNEL_PAGETABLE;
            memoryInfo->Regions[regionCount].ACPI = 0;
            regionCount++;

            page_tables = (uint8_t*)(uintptr_t)base;

            base += kernel_pageTableSize;
            if (length == kernel_pageTableSize) continue;
            length -= kernel_pageTableSize;
        }

        // Page Info
        if (!pageInfo_address && length >= pageInfo_size && type == MEMORY_TYPE_USABLE)
        {
            memoryInfo->Regions[regionCount].Begin = base;
            memoryInfo->Regions[regionCount].Length = pageInfo_size;
            memoryInfo->Regions[regionCount].Type = MEMORY_TYPE_KERNEL_PAGEINFO;
            memoryInfo->Regions[regionCount].ACPI = 0;
            regionCount++;

            pageInfo_address = (uint8_t*)(uintptr_t)base;

            base += pageInfo_size;
            if (length == pageInfo_size) continue;
            length -= pageInfo_size;
        }

        memoryInfo->Regions[regionCount].Begin = base;
        memoryInfo->Regions[regionCount].Length = length;
        memoryInfo->Regions[regionCount].Type = type;
        memoryInfo->Regions[regionCount].ACPI = acpi;

        lastEnd = base + length;
        regionCount++;
    }

    memoryInfo->RegionCount = regionCount;

    MemoryAddresses memoryAddresses;
    
    memoryAddresses.kernelAddress = kernel_address;

    memoryAddresses.pageDirectoryAddress = page_directory;

    memoryAddresses.pageTableCount = kernel_pageTableCount;
    memoryAddresses.pageTableAddress = page_tables;

    memoryAddresses.pageInfoAddress = pageInfo_address;
    memoryAddresses.pageInfoSize = pageInfo_size;

    return memoryAddresses;
}
