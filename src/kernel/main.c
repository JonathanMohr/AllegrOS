#include <bootparams.h>
#include <abi.h>
#include <stddef.h>
#include <stdint.h>

#include <minmax.h>
#include "arch/x86/irq/irq.h"
#include "arch/x86/isr.h"
#include "filesystem/filesystem.h"
#include "panic/panic.h"

#include "kconsole/kconsole.h"
#include "kconsole/format.h"

#include "memory/memory.h"

#include "arch/x86/x86.h"


#include "pci/pci.h"


#include "device/disk/ata/ata.h"

#include "device/partition/mbr.h"

#include "device/device.h"

#include "filesystem/fat/fat.h"
#include "filesystem/vfs.h"
#include "scheduler/scheduler.h"

#include "lock.h"

bool Timer_Handler(const Registers* regs);
static void Spawn(void* arg);

BootParams bootParams;

KernelConsole* mux_console;

uint32_t pciCount = 0;
PCI_Device* pciDevices;

uint64_t blockDeviceCount = 0;
Block_Device blockDevices[64]; /* 16 physical, 48 logical */

uint64_t fsDriverCount = 0;
Filesystem_Driver fsDrivers[8];

VFS vfs;

void CDECL kmain(BootParams* bParams)
{
    bootParams = *bParams;

    x86_Initialize();
    x86_PIT_Timer_Initialize(250, Timer_Handler);

    if (Memory_Initialize(&bootParams.Memory) != MEMORY_SUCCESS)
    {
        PanicMessage("[KERNEL] Could not initialize memory\n");
        Panic();
    }

    mux_console = KernelConsole_GetOutput();

    KernelConsole_ClearScreen(mux_console);
    KernelConsole_PutString(mux_console, "Hello world from kernel!\n");

    pciDevices = PCI_Scan(&pciCount);
    if (!pciDevices)
    {
        PanicMessage("[KERNEL] Could not scan PCI-Devices\n");
        Panic();
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
                    KernelConsole_PutString(mux_console, "Warning: Limit of physical block devices reached\n");
            }
        }
    }

    // Memory_KernelFree(pciDevices);

    const uint32_t physicalBlockDeviceCount = blockDeviceCount;
    for (uint32_t i = 0; i < physicalBlockDeviceCount; i++)
    {
        Block_Device* device = &blockDevices[i];
        size_t blockDeviceNameLen = 0;
        while (device->name[blockDeviceNameLen])
            blockDeviceNameLen++;
        const bool isMBR = MBR_CheckDisk(device);

        if (isMBR)
            KernelConsole_PrintFormat(mux_console, "MBR-partitioned block device %udd found\n", i + 1);

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
                        char nameBuffer[512] = {0};
                        const size_t nameLen = min(blockDeviceNameLen, (size_t)490);

                        for (size_t k = 0; k < nameLen; k++)
                            nameBuffer[k] = device->name[k];

                        nameBuffer[nameLen] = ':';
                        
                        char digits[21] = {0};
                        uint64_t n = j + 1;
                        size_t dIdx = 0;
                        do
                        {
                            digits[dIdx++] = '0' + (n % 10);
                            n /= 10;
                        } while (n && dIdx < 20);

                        for (size_t i = 0; i < dIdx; i++)
                            nameBuffer[nameLen + 1 + i] = digits[dIdx - 1 - i];

                        if (Device_PartitionTable_CreateBlockDevice(device, &blockDevices[blockDeviceCount], nameBuffer, start, count))
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
                        KernelConsole_PutString(mux_console, "Warning: Limit of logical block devices reached\n");
                }
                else
                    KernelConsole_PrintFormat(mux_console, "    %udq: Could not get sectors\n", j + 1);
            }
        }
    }

    for (uint32_t i = 0; i < blockDeviceCount; i++)
    {
        Block_Device* device = &blockDevices[i];

        KernelConsole_PrintFormat(mux_console, "Block-Device %udd: \"%s\"\n", i + 1, device->name);

        KernelConsole_PrintFormat(mux_console, "  Type: %s\n", device->type);

        KernelConsole_PrintFormat(mux_console, "  Block-Size: %uxqh\n", device->sectorSize);
        KernelConsole_PrintFormat(mux_console, "  Block-Count: %uxqh\n", device->sectorCount);
    }

    for (uint32_t i = 0; i < blockDeviceCount; i++)
    {
        Block_Device* device = &blockDevices[i];

        if (FAT_CheckDevice(device))
        {
            if (fsDriverCount < 8)
            {
                KernelConsole_PrintFormat(mux_console, "FAT filesystem found for block device %udd\n", i + 1);
                if (FAT_GetDriver(device, &fsDrivers[fsDriverCount]))
                    fsDriverCount++;
                else
                    KernelConsole_PrintFormat(mux_console, "Could not get FAT driver for block device %udd\n", i +1);
            }
            else
                KernelConsole_PutString(mux_console, "Warning: Limit of filesystems reached\n");
        }
        else
        {
            KernelConsole_PrintFormat(mux_console, "No filesystem found for block device %udd\n", i + 1);
        }
    }

    if (fsDriverCount == 0)
    {
        KernelConsole_PutString(mux_console, "Could not find a filesystem\n");
        goto end_before_vfs;
    }

    if (!VFS_Initialize(&vfs, &fsDrivers[0]))
    {
        KernelConsole_PutString(mux_console, "Could not initialize VFS\n");
        goto end;
    }

    VFS_File* file = VFS_File_Open(&vfs, NULL, "test.txt");
    if (!file)
    {
        KernelConsole_PutString(mux_console, "Could not open file\n");
        goto end;
    }

    uint64_t fileRead;
    char fileBuffer[512];
    while ((fileRead = VFS_File_Read(file, sizeof(fileBuffer), fileBuffer)))
    {
        for (uint64_t i = 0; i < fileRead; i++)
            KernelConsole_PutChar(mux_console, fileBuffer[i]);
    }

    VFS_File_Close(file);

    if (!Scheduler_AddTask(Spawn, NULL, MEMORY_KERNEL))
    {
        KernelConsole_PutString(mux_console, "Could not add initial kernel task\n");
        goto end_before_vfs;
    }
    __asm__ volatile("sti");

end:
    //VFS_Destroy(&vfs);
end_before_vfs:
    for(;;);
}

bool Timer_Handler(const Registers* regs)
{
    x86_IRQ_Send_EOI(0);
    Scheduler_Schedule();

    return false;
}

static void Greet(void* arg);

static void Spawn(void* arg)
{
    Kernel_Lock();
    KernelConsole_PutString(mux_console, "Spawning Task 1...\n");
    Kernel_Unlock();

    if (!Scheduler_AddTask(Greet, "Hello!", MEMORY_KERNEL))
    {
        Kernel_Lock();
        KernelConsole_PutString(mux_console, "Could not spawn Greet task 1\n");
        Kernel_Unlock();
        goto end;
    }

    Kernel_Lock();
    KernelConsole_PutString(mux_console, "Spawning Task 2...\n");
    Kernel_Unlock();

    if (!Scheduler_AddTask(Greet, "Greetings!", MEMORY_KERNEL))
    {
        Kernel_Lock();
        KernelConsole_PutString(mux_console, "Could not spawn Greet task 2\n");
        Kernel_Unlock();
        goto end;
    }

    Kernel_Lock();
    KernelConsole_PutString(mux_console, "Spawning Task 3...\n");
    Kernel_Unlock();

    if (!Scheduler_AddTask(Greet, "Good morning!", MEMORY_KERNEL))
    {
        Kernel_Lock();
        KernelConsole_PutString(mux_console, "Could not spawn Greet task 3\n");
        Kernel_Unlock();
        goto end;
    }

end:
    for(;;);
}

static void Greet(void* arg)
{
    const char* str = arg;
    while (1)
    {
        Kernel_Lock();
        KernelConsole_PutString(mux_console, str);
        KernelConsole_PutChar(mux_console, '\n');
        Kernel_Unlock();
    }
    for(;;);
}
