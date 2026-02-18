#include <abi.h>
#include <stdint.h>
#include <bootparams.h>

#include "io/io.h"
#include "memory/memdetect.h"

BootParams bootParams;

void CDECL start(uint32_t boot_drive, uint32_t* page_directory)
{
    IO_Init();

    bootParams.BootDevice = (uint8_t)boot_drive;
    Memory_Detect(&bootParams.Memory);

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

        IO_PrintFormat(stream, " region %udd:\n", i);
        IO_PrintFormat(stream, "\t%uxq - %uxq (%uxq)\n", memoryRegion->Begin, memoryRegion->Begin + memoryRegion->Length - 1, memoryRegion->Length);
        IO_PrintFormat(stream, "\tACPI: %udd\n", memoryRegion->ACPI);
    }

end:
    for (;;);
}
