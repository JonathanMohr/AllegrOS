#include <bootparams.h>
#include <abi.h>

#include "kconsole/kconsole.h"
#include "kconsole/format.h"

void CDECL kmain(BootParams* bParams)
{
    BootParams bootParams = *bParams;

    KernelConsole* vga_console = KernelConsole_GetVGA();
    KernelConsole* debug_console = KernelConsole_GetDebug();
    KernelConsole* kconsoles[2] = { vga_console, debug_console };
    KernelConsoleMux mux_kconsole;
    KernelConsole* mux_console = KernelConsole_InitMux(&mux_kconsole, kconsoles, sizeof(kconsoles) / sizeof(KernelConsole*));

    KernelConsole_ClearScreen(mux_console);
    KernelConsole_PutString(mux_console, "Hello world from kernel!\n");

    KernelConsole_PrintFormat(mux_console, "BootDevice: 0x%uxb\n", bootParams.BootDevice);

    KernelConsole_PutString(mux_console, "MemoryInfo:\n");
    KernelConsole_PrintFormat(mux_console, "  RegionCount: %udd\n", bootParams.Memory.RegionCount);
    for (uint32_t i = 0; i < bootParams.Memory.RegionCount; i++)
    {
        MemoryRegion* region = &bootParams.Memory.Regions[i];

        KernelConsole_PrintFormat(mux_console, "    Region %udd: %uxq - %uxq (%uxq)\n", i, region->Begin, region->Begin + region->Length - 1, region->Length);

        KernelConsole_PutString(mux_console, "      Type: ");

        switch (region->Type)
        {
            case MEMORY_TYPE_USABLE: KernelConsole_PutString(mux_console, "Usable"); break;
            case MEMORY_TYPE_RESERVED: KernelConsole_PutString(mux_console, "Reserved"); break;
            case MEMORY_TYPE_ACPI_RECLAIMABLE: KernelConsole_PutString(mux_console, "ACPI-Reclaimable"); break;
            case MEMORY_TYPE_ACPI_NVS: KernelConsole_PutString(mux_console, "ACPI-NVS"); break;
            case MEMORY_TYPE_BAD: KernelConsole_PutString(mux_console, "Bad"); break;

            case MEMORY_TYPE_RELUCTANT: KernelConsole_PutString(mux_console, "Reluctant"); break;
            case MEMORY_TYPE_HARDWARE: KernelConsole_PutString(mux_console, "Hardware"); break;
            case MEMORY_TYPE_KERNEL_PAGETABLE: KernelConsole_PutString(mux_console, "Kernel-Pagetable"); break;
            case MEMORY_TYPE_KERNEL: KernelConsole_PutString(mux_console, "Kernel"); break;

            default: KernelConsole_PutString(mux_console, "Invalid type"); break;
        }

        KernelConsole_PutChar(mux_console, '\n');

        KernelConsole_PrintFormat(mux_console, "      ACPI: %uxd \n", region->ACPI);
    }

end:
    for(;;);
}
