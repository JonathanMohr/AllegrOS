#include <bootparams.h>
#include <abi.h>
#include <stddef.h>
#include <stdint.h>
#include <minmax.h>

#include <memory.h>
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
#include "stdbool.h"

static bool Keyboard_Handler(const Registers* regs);
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
                                KernelConsole_PrintFormat(mux_console, "    %udq: Created block device %udd for sectors %uxqh - (including) %uxqh | Count: %uxqh\n", j + 1, blockDeviceCount + 1, start, start + count - 1, count);
                            else
                                KernelConsole_PrintFormat(mux_console, "    %udq: Created block device %udd for sector %uxqh\n", j + 1, blockDeviceCount + 1, start);
                            blockDeviceCount++;
                        }
                        else
                            KernelConsole_PrintFormat(mux_console, "    %udq: Could not create block device %udd for partition\n", j + 1, blockDeviceCount + 1, j + 1);
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

        KernelConsole_PrintFormat(mux_console, "  Block-Size: %uxdh\n", device->sectorSize);
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

    x86_IRQ_RegisterHandler(1, Keyboard_Handler);

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


#define KEYBOARD_RINGBUFFER_SIZE 256
struct syscall_keyboard_event keyboard_events[KEYBOARD_RINGBUFFER_SIZE];
volatile size_t keyboard_write_index = 0;
volatile size_t keyboard_read_index = 0;

static bool extended_pending = false;
static bool Keyboard_Handler(const Registers* regs)
{
    // TODO: Extremely ugly and bad, just redesign later
    (void)regs;

    IRQ_PushDisable();

    uint8_t scancode = x86_inb(0x60);

    if (scancode == 0xE0)
    {
        extended_pending = true;
        IRQ_PopDisable();
        return true;
    }

    const bool released = (scancode & 0x80) != 0;
    const uint8_t keycode = (scancode & 0x7F) | (extended_pending ? 0x80 : 0x00);

    const size_t next = (keyboard_write_index + 1) % KEYBOARD_RINGBUFFER_SIZE;
    if (next == keyboard_read_index)
    {
        PanicMessage("Keyboard ring buffer full!\n");
        extended_pending = false;
        IRQ_PopDisable();
        return true;
    }
    else
    {
        struct syscall_keyboard_event event;
        event.keycode = keycode;
        event.released = released;
        keyboard_events[keyboard_write_index] = event;
        keyboard_write_index = next;
    }

    extended_pending = false;
    IRQ_PopDisable();

    return true;
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
            // arg1: exit code
            KernelConsole_PrintFormat(mux_console, "Exit syscall: %udd\n", arg1);

            Kernel_Unlock();

            Scheduler_Exit();
            break;

        case SYSCALL_OPEN_FILE:
            // returns: handle
            // arg1: path
            // arg2: create (zero if not)
        {
            const char* path = (const char*)arg1;
            const int create = (int)arg2;

            VFS_File* file = VFS_File_Open(&vfs, NULL, path);
            if (!file && create)
            {
                if (VFS_Create(&vfs, NULL, path, FILESYSTEM_ENTRY_FILE, 0))
                    file = VFS_File_Open(&vfs, NULL, path);
            }

            if (!file)
                return_value = 0;
            else
                return_value = (syscall_t)file;

            break;
        }

        case SYSCALL_CLOSE_FILE:
            // returns: zero on success, non-zero on failure
            // arg1: handle
        {
            VFS_File* handle = (VFS_File*)arg1;
            VFS_File_Close(handle);
            return_value = 0;

            break;
        }

        case SYSCALL_OPEN_DIR:
            // returns: handle
            // arg1: path
        {
            const char* path = (const char*)arg1;

            VFS_File* file = VFS_Dir_Open(&vfs, NULL, path);
            if (!file)
                return_value = 0;
            else
                return_value = (syscall_t)file;

            break;
        }

        case SYSCALL_CLOSE_DIR:
            // returns: zero on success, non-zero on failure
            // arg1: handle
        {
            VFS_File* handle = (VFS_File*)arg1;
            VFS_Dir_Close(handle);
            return_value = 0;

            break;
        }

        case SYSCALL_READ:
            // returns: read
            // arg1: handle
            // arg2: data
            // arg3: len
        {
            VFS_File* handle = (VFS_File*)arg1;
            void* data = (void*)arg2;
            const uint64_t len = (uint64_t)arg3;

            if (arg1 == 0)
            {
                return_value = 0;
            }
            else
            {
                uint64_t read = VFS_File_Read(handle, len, data);
                return_value = (syscall_t)read;
            }

            break;
        }

        case SYSCALL_WRITE:
            // returns: written
            // arg1: handle
            // arg2: data
            // arg3: len
        {
            VFS_File* handle = (VFS_File*)arg1;
            const void* data = (const void*)arg2;
            const uint64_t len = (uint64_t)arg3;

            if (arg1 == 0)
            {
                const char* d = data;
                for (uint32_t i = 0; i < arg3; i++)
                    KernelConsole_PutChar(mux_console, d[i]);
                return_value = arg3;
            }
            else
            {
                uint64_t written = VFS_File_Write(handle, len, data);
                return_value = (syscall_t)written;
            }

            break;
        }

        case SYSCALL_READDIR:
            // returns: zero on success, non-zero on failure or when finished
            // arg1: handle
            // arg2: entryOut (Pointer to syscall_entry)
        {
            VFS_File* handle = (VFS_File*)arg1;
            struct syscall_entry* entryOut = (struct syscall_entry*)arg2;

            VFS_Entry* entry = VFS_Dir_Read(handle);
            if (!entry)
                return_value = 1;
            else
            {
                memcpy(entryOut->name, entry->name, FILESYSTEM_MAX_NAME);
                entryOut->name[FILESYSTEM_MAX_NAME] = '\0';

                entryOut->size = (unsigned long)entry->node->node.size;

                switch (entry->node->node.type)
                {
                    case FILESYSTEM_ENTRY_FILE: entryOut->type = SYSCALL_ENTRY_FILE; break;
                    case FILESYSTEM_ENTRY_DIRECTORY: entryOut->type = SYSCALL_ENTRY_DIRECTORY; break;
                }

                entryOut->attributes = 0;
                if (entry->node->node.attributes & FILESYSTEM_ATTRIBUTE_READONLY)
                    entryOut->attributes |= SYSCALL_ENTRY_READONLY;
                if (entry->node->node.attributes & FILESYSTEM_ATTRIBUTE_EXECUTABLE)
                    entryOut->attributes |= SYSCALL_ENTRY_EXECUTABLE;
                if (entry->node->node.attributes & FILESYSTEM_ATTRIBUTE_HIDDEN)
                    entryOut->attributes |= SYSCALL_ENTRY_HIDDEN;
                if (entry->node->node.attributes & FILESYSTEM_ATTRIBUTE_SYSTEM)
                    entryOut->attributes |= SYSCALL_ENTRY_SYSTEM;

                return_value = 0;
            }

            break;
        }

        case SYSCALL_MAKEDIR:
            // returns: zero on success, non-zero on failure or when finished
            // arg1: path
        {
            const char* path = (const char*)arg1;

            if (VFS_Create(&vfs, NULL, path, FILESYSTEM_ENTRY_DIRECTORY, 0))
                return_value = 0;
            else
                return_value = 1;

            break;
        }

        case SYSCALL_MOVE:
            // returns: zero on success, non-zero on failure or when finished
            // arg1: sourcePath
            // arg2: destinationPath
        {
            const char* srcPath = (const char*)arg1;
            const char* dstPath = (const char*)arg2;

            if (VFS_MoveEntry(&vfs, NULL, srcPath, dstPath, true))
                return_value = 0;
            else
                return_value = 1;

            break;
        }

        case SYSCALL_REMOVE:
            // returns: zero on success, non-zero on failure or when finished
            // arg1: path
        {
            const char* path = (const char*)arg1;

            if (VFS_Unlink(&vfs, NULL, path))
                return_value = 0;
            else
                return_value = 1;

            break;
        }


        case SYSCALL_ALLOCATE_PAGES:
            // returns: addr (0 if error)
            // arg1: addr (0 if dynamic)
            // arg2: len
            // arg3: flags
        {
            uintptr_t addr = arg1;
            const uintptr_t len = arg2;
            const syscall_memory_flags_t sFlags = (syscall_memory_flags_t)arg3;

            const uintptr_t pageCount = len / MEMORY_PAGE_SIZE;
            if (len % MEMORY_PAGE_SIZE != 0)
            {
                return_value = 0;
                break;
            }

            if (addr % MEMORY_PAGE_SIZE)
            {
                return_value = 0;
                break;
            }

            Memory_Flags flags = MEMORY_USER;
            if (sFlags & SYSCALL_MEMORY_READABLE)
                flags |= MEMORY_READABLE;
            if (sFlags & SYSCALL_MEMORY_WRITABLE)
                flags |= MEMORY_WRITABLE;
            if (sFlags & SYSCALL_MEMORY_EXECUTABLE)
                flags |= MEMORY_EXECUTABLE;

            if (addr)
            {
                Memory_Result result = Memory_ReserveVirtual(Memory_CurrentAddressSpace(), addr, pageCount, flags);
                if (result != MEMORY_SUCCESS)
                {
                    return_value = 0;
                    break;
                }
            }

            if (!addr)
            {
                Memory_Result result = Memory_AllocateVirtual(Memory_CurrentAddressSpace(), pageCount, flags, &addr);
                if (result != MEMORY_SUCCESS)
                {
                    return_value = 0;
                    break;
                }
            }

            Memory_Result result = Memory_LinkNew(Memory_CurrentAddressSpace(), addr, flags, pageCount);
            if (result != MEMORY_SUCCESS)
            {
                return_value = 0;
                result = Memory_FreeVirtual(Memory_CurrentAddressSpace(), addr, pageCount, true);
                if (result != MEMORY_SUCCESS)
                    PanicMessageInfo("Syscall_Handler", "Memory_FreeVirtual(Memory_CurrentAddressSpace(), %p, %p, true) failed\n", addr, pageCount);
                break;
            }

            return_value = addr;

            break;
        }
            
        case SYSCALL_FREE_PAGES:
            // returns: zero on success, non-zero on failure
            // arg1: addr
            // arg2: len
        {
            const uintptr_t addr = arg1;
            const uintptr_t len = arg2;

            const uintptr_t pageCount = len / MEMORY_PAGE_SIZE;
            if (len % MEMORY_PAGE_SIZE != 0)
            {
                return_value = 1;
                break;
            }

            Memory_Result result = Memory_Unlink(Memory_CurrentAddressSpace(), addr, pageCount);
            if (result != MEMORY_SUCCESS)
            {
                return_value = 1;
                break;
            }

            result = Memory_FreeVirtual(Memory_CurrentAddressSpace(), addr, pageCount, false);
            if (result != MEMORY_SUCCESS)
            {
                PanicMessageInfo("Syscall_Handler", "Memory_FreeVirtual(Memory_CurrentAddressSpace(), %p, %p, false) failed\n", addr, pageCount);
            }

            return_value = 0;

            break;
        }

        case SYSCALL_READ_KEYBOARD_EVENT:
            // returns: zero on success, non-zero on failure or when no event is available
            // arg1: eventOut (Pointer to syscall_keyboard_event)
        {
            const uintptr_t eventOut = arg1;
            struct syscall_keyboard_event* out = (struct syscall_keyboard_event*)eventOut;

            // TODO: Validate pointer

            if (keyboard_read_index == keyboard_write_index)
                return_value = 1;
            else
            {
                *out = keyboard_events[keyboard_read_index];
                keyboard_read_index = (keyboard_read_index + 1) % KEYBOARD_RINGBUFFER_SIZE;
                return_value = 0;
            }

            break;
        }


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
