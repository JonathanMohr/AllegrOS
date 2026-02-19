#include <abi.h>
#include <stdint.h>
#include <bootparams.h>

#include "io/io.h"
#include "memory/memdetect.h"
#include "x86/x86.h"
#include "disk/disk.h"
#include "mbr/mbr.h"

BootParams bootParams;

void CDECL start(uint32_t boot_drive, uint32_t* page_directory, x86_E820MemoryBlock* memoryBlocks, uint32_t memoryBlock_count)
{
    IO_Init();

    bootParams.BootDevice = (uint8_t)boot_drive;
    Memory_Detect(&bootParams.Memory, memoryBlocks, memoryBlock_count);

    stream_t stream = dbgout;

    IO_PrintFormat(stream, "BootDrive: 0x%uxb\n", bootParams.BootDevice);
    IO_PrintFormat(stream, "Page Directory: %p\n", page_directory);
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

            default:
                IO_PutString(stream, "Unknown");
                break;
        }

        IO_PrintFormat(stream, " region %udd:\n", i + 1);
        IO_PrintFormat(stream, "\t%uxq - %uxq (%uxq)\n", memoryRegion->Begin, memoryRegion->Begin + memoryRegion->Length - 1, memoryRegion->Length);
        IO_PrintFormat(stream, "\tACPI: %udd\n", memoryRegion->ACPI);
    }

    IO_PrintFormat(stream, "Filesystem sectors: %uxd - %uxd\n", MBR_GetFSStart(), MBR_GetFSStart() + MBR_GetFSSize() - 1);

    Disk disk;
    if (!Disk_Initialize(&disk, bootParams.BootDevice))
    {
        IO_PutStringCritical("Couldn't initialize disk!\n");
        goto end;
    }

    uint8_t buffer[512];

    if (!Disk_ReadSectors(&disk, 0, 1, buffer))
    {
        IO_PutString(vgaout, "Couldn't read sector 0!\n");
        goto end;
    }

    for (int i = 0; i < 512; i++)
    {
        IO_PrintFormat(dbgout, "%uxb%c", buffer[i], ((i + 1) % 16 == 0) ? '\n' : ' ');
    }

end:
    for (;;);
}
