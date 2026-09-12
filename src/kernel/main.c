#include <bootparams.h>
#include <abi.h>
#include <stddef.h>
#include <stdint.h>
#include <minmax.h>

#include <syscall_nums.h>

#include "arch/x86/irq/irq.h"
#include "arch/x86/isr.h"
#include "arch/x86/x86.h"
#include "arch/x86/usermode.h"

#include "elf/elf.h"
#include "memory/memory.h"
#include "memory/stack.h"

#include "kconsole/vga/vga.h"
#include "kconsole/kconsole.h"
#include "kconsole/format.h"

#include "memory/result.h"
#include "panic/panic.h"

#include "scheduler/scheduler.h"
#include "lock.h"

#include "pci/pci.h"

#include "device/disk/ata/ata.h"
#include "device/partition/mbr.h"
#include "device/device.h"

#include "filesystem/vfs.h"
#include "filesystem/fat/fat.h"

static syscall_t Syscall_Handler(const Registers* regs);
static bool Timer_Handler(const Registers* regs);
static void Spawn(void* arg);

BootParams bootParams;

KernelConsole* mux_console;

uint32_t pciCount = 0;
PCI_Device* pciDevices;

uint32_t blockDeviceCount = 0;
Block_Device blockDevices[64]; /* 16 physical, 48 logical */

uint64_t fsDriverCount = 0;
Filesystem_Driver fsDrivers[8];

VFS vfs;

void CDECL kmain(BootParams* bParams)
{
    bootParams = *bParams;

    x86_Initialize();

    const Memory_Result memoryInitializeResult = Memory_Initialize(&bootParams.Memory);
    if (memoryInitializeResult != MEMORY_SUCCESS)
    {
        PanicMessage("[KERNEL] Could not initialize memory: %rm\n");
        Panic();
    }

    if (!VGA_Initialize())
    {
        PanicMessage("[KERNEL] Could not initialize VGA\n");
        Panic();
    }

    TaskState* taskState = Scheduler_Initialize();
    if (!taskState)
    {
        PanicMessage("[KERNEL] Could not initialize scheduler\n");
        Panic();
    }

    Kernel_Lock();

    x86_PIT_Timer_Initialize(250, Timer_Handler);

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
            for (uint8_t j = 0; j < 4; j++)
            {
                if (blockDeviceCount < 16)
                {
                    const bool primary = j & 1;
                    const bool slave = j & 2;

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

                        for (size_t k = 0; k < dIdx; k++)
                            nameBuffer[nameLen + 1 + k] = digits[dIdx - 1 - k];

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

    KernelConsole_ClearScreen(mux_console);

    x86_ISR_RegisterHandler(0x80, Syscall_Handler);
    x86_ISR_SetUser(0x80, true);

    Kernel_Unlock();

    if (!Scheduler_AddTask(Spawn, NULL, MEMORY_KERNEL))
    {
        KernelConsole_PutString(mux_console, "Could not add initial kernel task\n");
        goto end_before_vfs;
    }

end:
    //VFS_Destroy(&vfs);
end_before_vfs:
    *taskState = TASK_IDLE;
    return;
}

static syscall_t Syscall_Handler(const Registers* regs)
{
    Kernel_Lock();

    const uint32_t number = regs->eax;
    const uint32_t arg1 = regs->ecx;
    const uint32_t arg2 = regs->edx;
    const uint32_t arg3 = regs->ebx;
    const uint32_t arg4 = regs->esi;
    const uint32_t arg5 = regs->edi;
    const uint32_t arg6 = regs->ebp;

    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    uint32_t return_value = regs->eax;
    switch (number)
    {
        case SYSCALL_EXIT:
            KernelConsole_PrintFormat(mux_console, "Exit syscall: %udd\n", arg1);
            Scheduler_Exit();
            break;

        case SYSCALL_WRITE:
            if (arg1 == 0)
            {
                const char* data = (const char*)arg2;
                for (uint32_t i = 0; i < arg3; i++)
                    KernelConsole_PutChar(mux_console, data[i]);
                return_value = arg3;
            }
            else return_value = 0;
            break;

        default:
            KernelConsole_PrintFormat(mux_console, "Unknown syscall %udd!\n", number);
    }

    Kernel_Unlock();

    return return_value;
}

static bool Timer_Handler(const Registers* regs)
{
    (void)regs;

    x86_IRQ_Send_EOI(0);
    Scheduler_Schedule();

    return false;
}

static void Process(void* arg);

static void Spawn(void* arg)
{
    (void)arg;

    Kernel_Lock();
    KernelConsole_PutString(mux_console, "Spawning Task 1...\n");

    AddressSpace* toolAddressSpace = NULL;
    Memory_Result addressSpaceResult = Memory_AddressSpace_Create(&toolAddressSpace);
    if (addressSpaceResult != MEMORY_SUCCESS)
    {
        KernelConsole_PutString(mux_console, "Could not create address space for tool\n");
        goto end;
    }
    Kernel_Unlock();

    if (!Scheduler_AddTask(Process, "/bin/tool", toolAddressSpace))
    {
        KernelConsole_PutString(mux_console, "Could not spawn Process task\n");
        goto end;
    }

end:
    Scheduler_Exit();
}

static void Process(void* arg)
{
    const char* filePath = arg;
    Kernel_Lock();

    KernelConsole_PrintFormat(mux_console, "Opening %s\n", filePath);
    
    VFS_File* file = VFS_File_Open(&vfs, NULL, filePath);
    if (!file)
    {
        PanicMessage("%s does not exist\n", filePath);
        Scheduler_Exit();
    }

    KernelConsole_PrintFormat(mux_console, "Creating stack for %s\n", filePath);

    uintptr_t stackTop;
    Memory_Result stackResult = AllocateUserStack(Memory_CurrentAddressSpace(), &stackTop);
    if (stackResult != MEMORY_SUCCESS)
    {
        PanicMessage("Stack creation for %s failed: %rm\n", filePath, stackResult);
        Scheduler_Exit();
    }

    KernelConsole_PrintFormat(mux_console, "Parsing ELF for %s\n", filePath);

    uintptr_t entryPoint;
    ELF_Result loadResult = ELF_Load(file, Scheduler_Current()->addressSpace, true, &entryPoint);
    if (loadResult != ELF_SUCCESS)
    {
        PanicMessage("ELF_Load for %s failed: %re\n", filePath, loadResult);
        Scheduler_Exit();
    }

    VFS_File_Close(file);

    KernelConsole_PrintFormat(mux_console, "Jumping to entry of %s\n", filePath);

    Kernel_Unlock();

    Arch_JumpToUserMode(entryPoint, stackTop);
}
