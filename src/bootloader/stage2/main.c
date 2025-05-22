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

    // load kernel
    KernelStart kernelEntry;

    uint32_t kernelSize = ELF_Size(&part, "/boot/kernel.elf");
    if (kernelSize == 0)
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    uint32_t kernelPages = (kernelSize + PAGE_SIZE - 1) / PAGE_SIZE + 1;
    uint32_t bootPages = (g_BootParams.Memory.Regions[0].Length + PAGE_SIZE - 1) / PAGE_SIZE;

    uint32_t mapCount = kernelPages + bootPages;
    uint32_t pageTables = (mapCount + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;
    uint32_t pagingSize = PAGE_SIZE + pageTables * PAGE_SIZE;

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

            MemoryRegion kernelRegion = *region;
            kernelRegion.Begin = alignedBegin;
            kernelRegion.Length = kernelPages * PAGE_SIZE;
            kernelRegion.Type = MEMORY_TYPE_KERNEL;
            newRegions[newCount++] = kernelRegion;

            MemoryRegion pagingRegion = *region;
            pagingRegion.Begin = kernelRegion.Begin + kernelRegion.Length;
            pagingRegion.Length = pagingSize;
            pagingRegion.Type = MEMORY_TYPE_RESERVED;
            newRegions[newCount++] = pagingRegion;

            MemoryRegion usableRegion = *region;
            usableRegion.Begin = pagingRegion.Begin + pagingRegion.Length;
            usableRegion.Length = (region->Begin + region->Length) - usableRegion.Begin;
            newRegions[newCount++] = usableRegion;

            continue;
        }

        newRegions[newCount++] = *region;
    }

    g_BootParams.Memory.RegionCount = newCount;
    for (int i = 0; i < newCount; ++i) {
        g_BootParams.Memory.Regions[i] = newRegions[i];
    }

    //TODO: identity map boot region, map kernelRegion to 0xC0000000, activate paging

    printf("Boot device: 0x%x\n", g_BootParams.BootDevice);
    printf("Memory region count: 0x%x\n", g_BootParams.Memory.RegionCount);
    for (int i = 0; i < g_BootParams.Memory.RegionCount; i++)
    {
        printf("MEM: start=0x%llx, length=0x%llx, type=%u\n",
            g_BootParams.Memory.Regions[i].Begin,
            g_BootParams.Memory.Regions[i].Length,
            g_BootParams.Memory.Regions[i].Type);
    }

    printf("Kernel page count: %u\n", kernelPages);

    uint32_t loadKernel = ELF_Read(&part, "/boot/kernel.elf", (void**)&kernelEntry);
    if (kernelSize == 0)
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    printf("Finished\n");

    // execute kernel
    kernelEntry(&g_BootParams);

end:
    for (;;);
}