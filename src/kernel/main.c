#include <bootparams.h>
#include <abi.h>
#include <stdint.h>

#include "disk/ata/ata.h"
#include "disk/disk.h"
#include "panic/panic.h"

#include "kconsole/kconsole.h"
#include "kconsole/format.h"

#include "memory/memory.h"
#include "memory/physical/manager.h"
#include "memory/virtual/manager.h"

#include "arch/x86/x86.h"

#include "arch/x86/paging.h"

#include "pci/pci.h"

void CDECL kmain(BootParams* bParams)
{
    BootParams bootParams = *bParams;

    x86_Initialize();

    if (!Memory_Initialize(&bootParams.Memory))
    {
        PanicMessage("[KERNEL] Could not initialize memory\n");
        Panic();
    }

    uint32_t pciCount;
    PCI_Device* pciDevices = PCI_Scan(&pciCount);
    if (!pciDevices)
    {
        PanicMessage("[KERNEL] Could not scan PCI-Devices\n");
        Panic();
    }

    KernelConsole* mux_console = KernelConsole_GetOutput();

    KernelConsole_ClearScreen(mux_console);
    KernelConsole_PutString(mux_console, "Hello world from kernel!\n");

    __asm__("sti");

    uint64_t blockDeviceCount = 0;
    Block_Device blockDevices[16];

    for (uint32_t i = 0; i < pciCount; i++)
    {
        const PCI_Device* device = &pciDevices[i];

        const bool isATA = ATA_CheckPCIDevice(device);
        if (isATA)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                if (blockDeviceCount >= 16)
                {
                    KernelConsole_PutString(mux_console, "Warning: Limit of block devices reached.\n");
                }
                else
                {
                    const bool primary = i & 1;
                    const bool slave = i & 2;

                    if (ATA_GetDiskFromPCIDevice(device, &blockDevices[blockDeviceCount], primary, slave))
                        blockDeviceCount++;
                }
            }
        }
    }

    for (uint32_t i = 0; i < pciCount; i++)
    {
        const PCI_Device* device = &pciDevices[i];

        KernelConsole_PrintFormat(mux_console, "PCI-Device %udd:\n", i + 1);
        KernelConsole_PrintFormat(mux_console, "  Vendor ID: %uxwh\n", device->vendorID);
        KernelConsole_PrintFormat(mux_console, "  Device ID: %uxwh\n", device->deviceID);

        KernelConsole_PrintFormat(mux_console, "  Class Code: %uxbh\n", device->classCode);
        KernelConsole_PrintFormat(mux_console, "  Subclass: %uxbh\n", device->subclass);
        KernelConsole_PrintFormat(mux_console, "  progIf: %uxbh\n", device->progIf);

        KernelConsole_PrintFormat(mux_console, "  Bus: %uxbh\n", device->bus);
        KernelConsole_PrintFormat(mux_console, "  Slot: %uxbh\n", device->slot);
        KernelConsole_PrintFormat(mux_console, "  Func: %uxbh\n", device->func);
    }

    for (uint32_t i = 0; i < blockDeviceCount; i++)
    {
        const Block_Device* device = &blockDevices[i];

        KernelConsole_PrintFormat(mux_console, "Block-Device %udd: \"%s\"\n", i + 1, device->name);

        KernelConsole_PrintFormat(mux_console, "  Type: %s\n", device->type);

        KernelConsole_PrintFormat(mux_console, "  Sector-Size: %uxqh\n", device->sectorSize);
        KernelConsole_PrintFormat(mux_console, "  Sector-Count: %uxqh\n", device->sectorCount);
    }

end:
    for(;;);
}
