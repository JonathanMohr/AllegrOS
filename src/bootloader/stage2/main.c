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

extern uint32_t* page_directory;

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

    uint32_t remainingKernelPages = ((kernelInfo.size + 0xFFF) & ~0xFFF) / 0x1000;
    uint32_t remainingPageInfoPages = (uint32_t)((addresses.pageInfoSize + 0xFFF) / 0x1000);

    uint32_t kernelPhysCursor = (uint32_t)addresses.kernelAddress;
    uint32_t pageInfoPhysCursor = (uint32_t)addresses.pageInfoAddress;

    for (uint32_t i = 0; i < addresses.pageTableCount; i++)
    {
        uint32_t ptPhysAddr = (uint32_t)addresses.pageTableAddress + i * 0x1000;
        uint32_t* pageTable = (uint32_t*)(0xFFC00000 + (768 + i) * 0x1000);

        page_directory[768 + i] = ptPhysAddr | 0x3;
        x86_invlpg(pageTable);

        for (uint32_t j = 0; j < 1024; j++)
        {
            if (remainingKernelPages > 0)
            {
                pageTable[j] = kernelPhysCursor | 0x3;
                kernelPhysCursor += 0x1000;
                remainingKernelPages--;
            }
            else if (remainingPageInfoPages > 0)
            {
                pageTable[j] = pageInfoPhysCursor | 0x3;
                pageInfoPhysCursor += 0x1000;
                remainingPageInfoPages--;
            }
            else
            {
                break;
            }
        }
    }

    bootParams.Memory.physPageArray = (void*)((uintptr_t)(0xc0000000 + ((kernelInfo.size + 0xFFF) & ~0xFFF)));
    bootParams.Memory.startOfFree = addresses.startOfFree;
    bootParams.Memory.initialAddressSpace = (uphysptr_t)page_directory;

    uint8_t* kernelEntry = ELF_Load(&partition, "/sys/kernel.elf");

    enterKernel(&bootParams, kernelEntry);

end:
    for (;;);
}
