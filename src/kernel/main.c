#include <bootparams.h>
#include <abi.h>
#include <stdint.h>

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

    for (uint32_t i = 0; i < pciCount; i++)
    {
        const PCI_Device* device = &pciDevices[i];
        KernelConsole_PrintFormat(mux_console, "Device %udd:\n", i);
        KernelConsole_PrintFormat(mux_console, "  Vendor ID: %uxw\n", device->vendorID);
        KernelConsole_PrintFormat(mux_console, "  Device ID: %uxw\n", device->deviceID);

        KernelConsole_PrintFormat(mux_console, "  Class Code: %uxb\n", device->classCode);
        KernelConsole_PrintFormat(mux_console, "  Subclass: %uxb\n", device->subclass);
        KernelConsole_PrintFormat(mux_console, "  progIf: %uxb\n", device->progIf);

        KernelConsole_PrintFormat(mux_console, "  Bus: %uxb\n", device->classCode);
        KernelConsole_PrintFormat(mux_console, "  Slot: %uxb\n", device->subclass);
        KernelConsole_PrintFormat(mux_console, "  Func: %uxb\n", device->progIf);
    }

end:
    for(;;);
}
