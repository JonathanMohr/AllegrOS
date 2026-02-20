#include <abi.h>
#include <stdint.h>
#include <bootparams.h>

#include "io/io.h"
#include "memory/memdetect.h"
#include "x86/x86.h"
#include "disk/disk.h"
#include "disk/partition.h"
#include "fat/fat.h"
#include "elf/elf.h"
#include "kernel.h"

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

    ELFLoadInfo kernelInfo = ELF_GetSize(&partition, "/sys/kernel.elf");
    if (!kernelInfo.size)
    {
        IO_PutStringCritical("Couldn't get kernel size!\n");
        goto end;
    }

    uint8_t* kernelAddress = (uint8_t*)0xC0000000;
    if (kernelInfo.start != (uint64_t)(uintptr_t)kernelAddress)
    {
        IO_PutStringCritical("Invalid kernel start!\n");
        goto end;
    }

    MemoryAddresses addresses = Memory_Detect(&bootParams.Memory, memoryBlocks, memoryBlock_count, kernelInfo.size);
    if (!addresses.pageTableAddress || !addresses.kernelAddress)
    {
        IO_PutStringCritical("Not enough memory for kernel!\n");
        goto end;
    }

    // map page directory to 0xFFFFF000
    page_directory_phys[1023] = (uint32_t)page_directory_phys | 0x3;
    x86_invlpg((void*)0xFFFFF000);
    uint32_t* page_directory = (uint32_t*)0xFFFFF000;

    uint32_t remainingPages = ((kernelInfo.size + 0xFFF) & ~0xFFF) / 0x1000;
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

    uint8_t* kernelEntry = ELF_Load(&partition, "/sys/kernel.elf");

    enterKernel(&bootParams, kernelEntry);

end:
    for (;;);
}
