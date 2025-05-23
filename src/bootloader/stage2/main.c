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
#include <boot/bootparams.h>
#include <core/memory/memory.h>
#include <stddef.h>

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

BootParams g_BootParams;

// I don't know why but it only works when using 0x100000 and not PAGE_SIZE
#define BOOTSIZE 0x100000

// Don't change the kernel linker virt addr
#define KERNEL_VIRT 0xC0000000

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

    // prepare paging
    uint32_t kernelSize = ELF_Size(&part, "/boot/kernel.elf");
    if (kernelSize == 0)
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    // prepare paging
    uint8_t* pageDirectoryPtr = NULL;
    uint32_t kernelPages = 0;
    uint8_t* kernelBegin = NULL;
    uint32_t bootLength = i686_prepare_paging(&g_BootParams.Memory, BOOTSIZE, kernelSize,
                                              &pageDirectoryPtr, &kernelPages, &kernelBegin);

    // initialize paging
    bootLength = (bootLength + BOOTSIZE - 1) / BOOTSIZE;
    PageDirectory* pageDirectory = i686_paging_Initialize(bootLength * BOOTSIZE, pageDirectoryPtr, KERNEL_VIRT, kernelPages, kernelBegin);

    // enable paging
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