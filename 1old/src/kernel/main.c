#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "hal/hal.h"
#include "timer.h"
#include "debug.h"
#include <boot/bootparams.h>
#include <core/memory/memory.h>
#include "memory/memory.h"
#include "stack/stack.h"
#include "stack/user.h"
#include "hal/paging.h"
#include "exceptions/exceptions.h"
#include "drivers/disk/disk.h"
#include "drivers/disk/mbr.h"
#include "drivers/fat/fat.h"
#include "drivers/elf/elf.h"
#include "syscalls/syscall.h"
#include "drivers/keyboard/keyboard.h"

//TODO add to HAL
#include "arch/i686/user.h"

#define ENTRY __attribute__((section(".entry")))

extern void _init();

extern uint8_t __text_start[];
extern uint8_t __data_start[];
extern uint8_t __rodata_start[];
extern uint8_t __bss_start[];
extern uint8_t __end[];

static BootParams bootParams;
static PageDirectory kernelPageDir;

__attribute__((section(".stack"))) 
static uint8_t kernel_stack[KERNEL_STACK_SIZE];

void crash_me();

void ENTRY start(BootParams* bParams)
{
    // copy bootParams to kernel
    bootParams = *bParams;

    // initialize stack top
    kernelStackTop = (uint32_t)&kernel_stack[KERNEL_STACK_SIZE];

    // initialize stack
    void* kernel_stack_end = &kernel_stack[KERNEL_STACK_SIZE];
    set_Stack((uint32_t)kernel_stack_end);
}

void kernel_main()
{
    // call global constructors
    _init();

    // Initialize
    memory_Initialize(&bootParams.Memory, bootParams.kernelSize);
    MemoryRegion* oldRegions = bootParams.Memory.Regions;
    bootParams.Memory.Regions = memory_Allocate(
        sizeof(MemoryRegion) * bootParams.Memory.RegionCount,
        1
    );
    memcpy(
        bootParams.Memory.Regions,
        oldRegions,
        sizeof(MemoryRegion) * bootParams.Memory.RegionCount
    );
    kernelPageDir = Paging_Initialize();
    if (!kernelPageDir.directory_virtual)
    {
        log_err("Kernel", "Couldn't initialize paging!");
        printf("Couldn't initialize paging!\nTry restarting.\n");
        goto end;
    }

    HAL_Initialize();

    uint16_t* buffer = (uint16_t*)memory_Allocate(256 * sizeof(uint16_t), 1);
    if (!buffer)
    {
        log_err("Kernel", "Couldn't allocate memory for buffer");
        printf("You're system doesn't have enough memory.\n");
        goto end;
    }
    Disk disk;
    if (!disk_Initialize(&disk, bootParams.BootDevice, buffer))
    {
        log_err("Kernel", "Couldn't initialize disk!");
        printf("Couldn't initialize disk drivers!\nTry restarting.\n");
        goto end;
    }

    Partition partition;
    MBR_DetectPartition(&partition, &disk, bootParams.partitionOffset, bootParams.partitionSize);
    if (!FAT_Initialize(&partition))
    {
        log_err("Kernel", "Couldn't initialize FAT!");
        printf("Couldn't initialize fat drivers!\nTry reinstalling OS.\n");
        goto end;
    }

    syscall_Init(&partition);

    // Initialize handlers
    isr_registerExceptionHandlers();
    i686_IRQ_RegisterHandler(0, timer);
    i686_IRQ_RegisterHandler(1, keyboard_handler);

    // log stuff
    log_debug("Drive", "Boot device: 0x%x", bootParams.BootDevice);
    log_debug("Drive", "Cylinders: 0x%x, Sectors: 0x%x, Heads: 0x%x",
        partition.disk->cylinders, partition.disk->sectors, partition.disk->heads);
    log_debug("Drive", "Serial number: '%s'", partition.disk->serial_number);
    log_debug("Drive", "Firmware revision: '%s'", partition.disk->firmware_revision);
    log_debug("Drive", "Model number: '%s'", partition.disk->model_number);
    log_debug("Drive", "Max sectors per read/write: 0x%x", partition.disk->max_sectors_per_rw);
    log_debug("Drive", "Total sectors 28 bit: 0x%x, Total sectors 48 bit: 0x%x",
        partition.disk->total_sectors_28bit, partition.disk->total_sectors_48bit);

    log_debug("Memory", "Memory region count: 0x%x", bootParams.Memory.RegionCount);
    for (int i = 0; i < bootParams.Memory.RegionCount; i++)
    {
        log_debug("Memory", "start=0x%llx, length=0x%llx, type=%lu",
            bootParams.Memory.Regions[i].Begin,
            bootParams.Memory.Regions[i].Length,
            bootParams.Memory.Regions[i].Type);
    }

    // start terminal
    PageDirectory user = Paging_Create(&kernelPageDir);

    Paging_Load(&user);
    uint8_t* stackTop = prepareUserStack();

    void* entry;
    uint32_t loadProgram = ELF_Read(&partition, "system/terminal.elf", &entry);
    if (loadProgram == 0)
    {
        log_err("Kernel", "ELF read failed.");
    }

    enter(entry, stackTop);

end:
    for (;;);
}