#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "mbr.h"
#include "elf.h"
#include "memdetect.h"
#include "paging/paging.h"
#include "memory/memory.h"
#include <boot/bootparams.h>
#include <core/memory/memory.h>
#include <stddef.h>

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

BootParams g_BootParams;

typedef void (*KernelStart)(BootParams* bootParams);

void __attribute__((cdecl)) start(uint16_t bootDrive, void* partition)
{
    clrscr();

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    Partition part;
    MBR_DetectPartition(&part, &disk, partition);

    if (!FAT_Initialize(&part))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // prepare boot params
    g_BootParams.BootDevice = bootDrive;
    Memory_Detect(&g_BootParams.Memory);

    Memory_AddBootRegion(&g_BootParams.Memory);

    // prepare paging
    uint32_t kernelSize = ELF_Size(&part, "/boot/kernel.elf");
    if (kernelSize == 0)
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    uint32_t kernelPages = (kernelSize + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t kernelPageTables = (kernelPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES + 1;
    uint32_t kernelPagingSize = kernelPageTables * PAGE_SIZE;
    //TODO: remove hardcoded 0x100000
    uint32_t bootPages = (0x100000 + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t bootPageTables = (bootPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;
    uint32_t bootPagingSize = bootPageTables * PAGE_SIZE;

    uint32_t pagingSize = 2 * PAGE_SIZE + kernelPagingSize + bootPagingSize;

    uint8_t* kernelBegin = NULL;
    uint8_t* pageDirectoryPtr = NULL;

    MemoryRegion pagingRegion;

    uint32_t bootLength = 0;

    memset(newRegions, 0, sizeof(newRegions));
    int newCount = 0;

    for (int i = 0; i < g_BootParams.Memory.RegionCount; ++i)
    {
        MemoryRegion* region = &g_BootParams.Memory.Regions[i];
        uint64_t begin = region->Begin;
        uint64_t end = region->Begin + region->Length;

        uintptr_t alignedBegin = (region->Begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Auf nächste PAGE_SIZE ausrichten
        uint32_t padding = alignedBegin - region->Begin;
        
        if (region->Type == MEMORY_TYPE_USABLE && region->Length >= (kernelPages * PAGE_SIZE + pagingSize + padding))
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

            pageDirectoryPtr = (uint8_t*)(uintptr_t)pagingRegion.Begin;

            MemoryRegion kernelRegion = *region;
            kernelRegion.Begin = pagingRegion.Begin + pagingRegion.Length;
            kernelRegion.Length = kernelPages * PAGE_SIZE;
            kernelRegion.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernelRegion;

            kernelBegin = (uint8_t*)(uintptr_t)kernelRegion.Begin;

            MemoryRegion usableRegion = *region;
            usableRegion.Begin = kernelRegion.Begin + kernelRegion.Length;
            usableRegion.Length = (region->Begin + region->Length) - usableRegion.Begin;
            newRegions[newCount++] = usableRegion;

            continue;
        }
        else if (region->Type == MEMORY_TYPE_BOOT)
        {
            bootLength = region->Length;
        }

        newRegions[newCount++] = *region;
    }

    g_BootParams.Memory.RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        g_BootParams.Memory.Regions[i] = newRegions[i];
    }

    PageDirectory pageDirectory;

    //TODO: remove hardcoded 0x100000
    uint8_t* newPtr = (uint8_t*)i686_paging_Initialize(&pageDirectory, 0x100000, pageDirectoryPtr);

    //TODO: remove hardcoded 0xC0000000
    uintptr_t kernelVirt = 0xC0000000;
    int numKernelTables = (kernelPages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    int kernelPDIndexStart = (kernelVirt >> 22);

    for (int i = 0; i < numKernelTables; i++) {
        pageDirectory.tables_virtual[kernelPDIndexStart + i] = (uint32_t*)newPtr;
        uint32_t* pt = (uint32_t*)newPtr;  // Deine Kernel-Page Tables (physisch oder virtuell je nach Setup)
        newPtr += PAGE_SIZE;
        memset(pt, 0, PAGE_SIZE);

        uintptr_t phys = (uintptr_t)pt;  // Falls virtuell, musst du hier noch zur physischen Adresse umrechnen

        // Page Directory Index berechnen
        uint32_t pdIndex = ((kernelVirt >> 22) + i) & 0x3FF;

        pageDirectory.directory[pdIndex] = phys | PAGE_PRESENT | PAGE_RW;

        kernelVirt += PAGE_SIZE * PAGE_TABLE_ENTRIES;  // Nächster 4MB-Bereich
    }

    uintptr_t physPage = (uintptr_t)kernelBegin;
    for (int pt = 0; pt < numKernelTables; pt++) {
        uint32_t* ptEntries = pageDirectory.tables_virtual[kernelPDIndexStart + pt];

        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            if ((pt * PAGE_TABLE_ENTRIES + i) >= kernelPages) {
                break; // Nicht mehr Seiten als KernelSize
            }

            ptEntries[i] = physPage | PAGE_PRESENT | PAGE_RW;
            physPage += PAGE_SIZE;
        }
    }

    //TODO: map page directory and page tables

    i686_enable_paging();

    // load kernel
    KernelStart kernelEntry;
    uint32_t loadKernel = ELF_Read(&part, "/boot/kernel.elf", (void**)&kernelEntry);
    if (loadKernel == 0)
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    // execute kernel
    kernelEntry(&g_BootParams);

end:
    for (;;);
}