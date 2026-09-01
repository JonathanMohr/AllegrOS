#include <bootparams.h>
#include <abi.h>
#include <stdint.h>

#include "panic/panic.h"

#include "kconsole/kconsole.h"
#include "kconsole/format.h"

#include "memory/memory.h"

#include "arch/x86/x86.h"

#include "arch/x86/paging.h"


#include "pci/pci.h"


#include "device/disk/ata/ata.h"

#include "device/partition/mbr.h"

#include "device/device.h"

#include "filesystem/fat/fat.h"



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
    Block_Device blockDevices[64]; /* 16 physical, 48 logical */

    for (uint32_t i = 0; i < pciCount; i++)
    {
        const PCI_Device* device = &pciDevices[i];

        const bool isATA = ATA_CheckPCIDevice(device);
        if (isATA)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                if (blockDeviceCount < 16)
                {
                    const bool primary = i & 1;
                    const bool slave = i & 2;

                    const int ata_result = ATA_GetDiskFromPCIDevice(device, &blockDevices[blockDeviceCount], primary, slave);
                    if (ata_result == ATA_SUCCESS)
                    {
                        KernelConsole_PrintFormat(mux_console, "Created block device %udq for ATA disk\n", blockDeviceCount + 1);
                        blockDeviceCount++;
                    }
                    else if (ata_result == ATA_ERROR)
                        KernelConsole_PrintFormat(mux_console, "Warning: Could not create block device %udq for partition\n", blockDeviceCount + 1);
                }
                else
                    KernelConsole_PutString(mux_console, "Warning: Limit of physical block devices reached.\n");
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

    Memory_KernelFree(pciDevices);

    const uint32_t physicalBlockDeviceCount = blockDeviceCount;
    for (uint32_t i = 0; i < physicalBlockDeviceCount; i++)
    {
        Block_Device* device = &blockDevices[i];
        const bool isMBR = MBR_CheckDisk(device);

        KernelConsole_PrintFormat(mux_console, "Block-Device %udd: \"%s\"\n", i + 1, device->name);

        KernelConsole_PrintFormat(mux_console, "  Type: %s\n", device->type);

        KernelConsole_PrintFormat(mux_console, "  Sector-Size: %uxqh\n", device->sectorSize);
        KernelConsole_PrintFormat(mux_console, "  Sector-Count: %uxqh\n", device->sectorCount);

        Device_PartitionTable table;
        if (!isMBR || !MBR_GetPartitionTable(device, &table))
            KernelConsole_PutString(mux_console, "  Partitions: Partition table could not be built\n");
        else
        {
            const uint64_t partitionCount = table.getPartitionCount(&table);
            for (uint64_t j = 0; j < partitionCount; j++)
            {
                uint64_t start;
                uint64_t count;
                if (table.getPartitionEntry(&table, j, &start, &count))
                {
                    if (blockDeviceCount < 64)
                    {
                        if (Device_PartitionTable_CreateBlockDevice(device, &blockDevices[blockDeviceCount], "Partition", start, count))
                        {
                            if (count > 1)
                                KernelConsole_PrintFormat(mux_console, "    %udq: Created block device %udq for sectors %uxqh - (including) %uxqh | Count: %uxqh\n", j + 1, blockDeviceCount + 1, start, start + count - 1, count);
                            else
                                KernelConsole_PrintFormat(mux_console, "    %udq: Created block device %udq for sector %uxqh\n", j + 1, blockDeviceCount + 1, start);
                            blockDeviceCount++;
                        }
                        else
                            KernelConsole_PrintFormat(mux_console, "    %udq: Could not create block device %udq for partition\n", j + 1, blockDeviceCount + 1, j + 1);
                    }
                    else
                        KernelConsole_PutString(mux_console, "Warning: Limit of logical block devices reached.\n");
                }
                else
                    KernelConsole_PrintFormat(mux_console, "    %udq: Could not get sectors\n", j + 1);
            }
        }
    }

end:
    for(;;);
}
