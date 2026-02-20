#include <abi.h>
#include <stdint.h>
#include <bootparams.h>

#include "io/io.h"
#include "memory/memdetect.h"
#include "x86/x86.h"
#include "disk/disk.h"
#include "disk/partition.h"
#include "fat/fat.h"

BootParams bootParams;

void CDECL start(uint32_t boot_drive, uint32_t* page_directory_phys, x86_E820MemoryBlock* memoryBlocks, uint32_t memoryBlock_count)
{
    IO_Init();

    bootParams.BootDevice = (uint8_t)boot_drive;

    Disk disk;
    if (!Disk_Initialize(&disk, bootParams.BootDevice))
    {
        IO_PutStringCritical("Couldn't initialize disk!\n");
        goto end;
    }

    Partition partition;
    Partition_GetFSPartition(&partition, &disk);

    if (!FAT_Initialize(&partition))
    {
        IO_PutStringCritical("Couldn't initialize FAT!\n");
        goto end;
    }

    FAT_File* kernel = FAT_Open(&partition, "/sys/kernel.bin");
    if (!kernel)
    {
        IO_PutStringCritical("Couldn't open kernel!\n");
        goto end;
    }

    MemoryAddresses addresses = Memory_Detect(&bootParams.Memory, memoryBlocks, memoryBlock_count, kernel->size);

    // map page directory to 0xFFFFF000
    page_directory_phys[1023] = (uint32_t)page_directory_phys | 0x3;
    x86_invlpg((void*)0xFFFFF000);
    uint32_t* page_directory = (uint32_t*)0xFFFFF000;

    uint32_t remainingPages = ((kernel->size + 0xFFF) & ~0xFFF) / 0x1000;
    for (uint32_t i = 0; i < addresses.pageTableCount; i++)
    {
        uint32_t ptPhysAddr = (uint32_t)addresses.pageTableAddress + i * 0x1000;
        uint32_t* pageTable = (uint32_t*)(0xFFC00000 + (768 + i) * 0x1000);

        page_directory[768 + i] = ptPhysAddr | 0x3;
        x86_invlpg(pageTable);

        for (uint32_t j = 0; j < 1024 && remainingPages > 0; j++, remainingPages--)
        {
            uint32_t physAddr = (uint32_t)addresses.kernelAddress + (i * 0x400000) + (j * 0x1000);
            pageTable[j] = physAddr | 0x3;
        }
    }

    stream_t stream = dbgout;
    IO_PrintFormat(stream, "BootDrive: 0x%uxb\n", bootParams.BootDevice);
    for (uint32_t i = 0; i < bootParams.Memory.RegionCount; i++)
    {
        const MemoryRegion* memoryRegion = &bootParams.Memory.Regions[i];

        switch (memoryRegion->Type)
        {
            case MEMORY_TYPE_USABLE:
                IO_PutString(stream, "Usable");
                break;

            case MEMORY_TYPE_RESERVED:
                IO_PutString(stream, "Reserved");
                break;

            case MEMORY_TYPE_ACPI_RECLAIMABLE:
                IO_PutString(stream, "ACPI-Reclaimable");
                break;

            case MEMORY_TYPE_ACPI_NVS:
                IO_PutString(stream, "ACPI-NVS");
                break;

            case MEMORY_TYPE_BAD:
                IO_PutString(stream, "Bad");
                break;

            case MEMORY_TYPE_RELUCTANT:
                IO_PutString(stream, "Reluctant");
                break;

            case MEMORY_TYPE_HARDWARE:
                IO_PutString(stream, "Hardware");
                break;

            case MEMORY_TYPE_KERNEL_PAGETABLE:
                IO_PutString(stream, "Kernel page table");
                break;

            case MEMORY_TYPE_KERNEL:
                IO_PutString(stream, "Kernel");
                break;

            default:
                IO_PutString(stream, "Unknown");
                break;
        }

        IO_PrintFormat(stream, " region %udd:\n", i + 1);
        IO_PrintFormat(stream, "\t%uxq - %uxq (%uxq)\n", memoryRegion->Begin, memoryRegion->Begin + memoryRegion->Length - 1, memoryRegion->Length);
        IO_PrintFormat(stream, "\tACPI: %udd\n", memoryRegion->ACPI);
    }

end:
    for (;;);
}
