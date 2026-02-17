#include "paging.h"

#include <core/Defs.h>
#include <core/memory/memory.h>
#include <stddef.h>
#include "../stdio.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL invlpg(uint32_t addr);

uint64_t i686_get_paging_size(uint64_t size)
{
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t page_tables = (pages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    uint64_t total_size = PAGE_SIZE + page_tables * PAGE_SIZE;

    return total_size;
}

PageDirectory* i686_paging_Initialize(uint64_t bootLength,
                                      void* ptr,
                                      uintptr_t kernelVirt,
                                      uint32_t kernelPages,
                                      uint8_t* kernelBegin)
{
    PageDirectory dir;

    uint64_t paging_size = i686_get_paging_size(bootLength);

    if (!ptr) {
        printf("No memory left for paging!");
        return NULL;
    }

    dir.directory = (uint32_t*)ptr;
    int num_page_tables = (int)((paging_size - PAGE_SIZE) / PAGE_SIZE);

    memset(dir.directory, 0, PAGE_SIZE);

    uint64_t pages = (bootLength + PAGE_SIZE - 1) / PAGE_SIZE;

    for (int pt = 0; pt < num_page_tables; pt++) {
        uint32_t* pt_entries = (uint32_t*)((uint8_t*)ptr + PAGE_SIZE + pt * PAGE_SIZE);
        memset(pt_entries, 0, PAGE_SIZE);

        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            int page_index = pt * PAGE_TABLE_ENTRIES + i;
            if (page_index >= pages) break;

            pt_entries[i] = (page_index * PAGE_SIZE) | (PAGE_PRESENT | PAGE_RW);
        }

        dir.directory[pt] = ((uint32_t)pt_entries) | (PAGE_PRESENT | PAGE_RW);
    }

    uintptr_t paging_base = (uintptr_t)ptr;
    uintptr_t paging_end = paging_base + paging_size;


    uint8_t* newPtr = (uint8_t*)paging_end;

    int numKernelTables = (kernelPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    int kernelPDIndexStart = (kernelVirt >> 22);

    uintptr_t physPage = (uintptr_t)kernelBegin;
    for (int i = 0; i < numKernelTables; i++) {
        uint32_t* pt = (uint32_t*)newPtr;
        newPtr += PAGE_SIZE;
        memset(pt, 0, PAGE_SIZE);

        uintptr_t phys = (uintptr_t)pt;

        uint32_t pdIndex = ((kernelVirt >> 22) + i) & 0x3FF;

        dir.directory[pdIndex] = phys | PAGE_PRESENT | PAGE_RW;

        kernelVirt += PAGE_SIZE * PAGE_TABLE_ENTRIES;

        for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            if ((i * PAGE_TABLE_ENTRIES + j) >= kernelPages) {
                break;
            }

            pt[j] = physPage | PAGE_PRESENT | PAGE_RW;
            physPage += PAGE_SIZE;
        }
    }

    // map page directory and page tables 0xFFC00000
    dir.directory[1023] = (uint32_t)dir.directory | PAGE_PRESENT | PAGE_RW;

    write_cr3((uint32_t)dir.directory);
}

void i686_enable_paging()
{
    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000; // Setze das Paging-Bit (bit 31)
    write_cr0(cr0);
}

uint32_t i686_prepare_paging(MemoryInfo* memInfo,
                             uint32_t bootSize,
                             uint32_t kernelSize,
                             uint8_t** pageDirectoryPtr,
                             uint32_t* kernelPages,
                             uint8_t** kernelBegin)
{
    *kernelPages = (kernelSize + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t kernelPageTables = (*kernelPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;
    uint32_t kernelPagingSize = kernelPageTables * PAGE_SIZE;

    uint32_t bootPages = (bootSize + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t bootPageTables = (bootPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;
    uint32_t bootPagingSize = bootPageTables * PAGE_SIZE;

    uint32_t pagingSize = PAGE_SIZE + kernelPagingSize + bootPagingSize;

    MemoryRegion pagingRegion;

    uint32_t bootLength = 0;

    MemoryRegion newRegions[MAX_REGIONS];
    memset(newRegions, 0, sizeof(newRegions));
    int newCount = 0;

    for (int i = 0; i < memInfo->RegionCount; ++i)
    {
        MemoryRegion* region = &memInfo->Regions[i];
        uint64_t begin = region->Begin;
        uint64_t end = region->Begin + region->Length;

        uintptr_t alignedBegin = (region->Begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        uint32_t padding = alignedBegin - region->Begin;
        
        if (region->Begin == 0)
        {
            MemoryRegion bootRegion = *region;
            bootRegion.Type = MEMORY_TYPE_BOOT;
            newRegions[newCount++] = bootRegion;

            bootLength = region->Length;

            continue;
        }
        else if (region->Type == MEMORY_TYPE_USABLE && region->Length >= (*kernelPages * PAGE_SIZE + pagingSize + padding))
        {
            if (padding > 0)
            {
                MemoryRegion paddingRegion = *region;
                paddingRegion.Type = MEMORY_TYPE_RESERVED;
                paddingRegion.Length = padding;
                newRegions[newCount++] = paddingRegion;
            }

            pagingRegion = *region;
            pagingRegion.Begin = alignedBegin;
            pagingRegion.Length = pagingSize;
            pagingRegion.Type = MEMORY_TYPE_RESERVED;
            newRegions[newCount++] = pagingRegion;

            *pageDirectoryPtr = (uint8_t*)(uintptr_t)pagingRegion.Begin;

            MemoryRegion kernelRegion = *region;
            kernelRegion.Begin = pagingRegion.Begin + pagingRegion.Length;
            kernelRegion.Length = *kernelPages * PAGE_SIZE;
            kernelRegion.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernelRegion;

            *kernelBegin = (uint8_t*)(uintptr_t)kernelRegion.Begin;

            MemoryRegion usableRegion = *region;
            usableRegion.Begin = kernelRegion.Begin + kernelRegion.Length;
            usableRegion.Length = (region->Begin + region->Length) - usableRegion.Begin;
            newRegions[newCount++] = usableRegion;

            continue;
        }
        
        newRegions[newCount++] = *region;
    }

    memInfo->RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        memInfo->Regions[i] = newRegions[i];
    }

    return bootLength;
}