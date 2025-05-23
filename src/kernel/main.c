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

#define ENTRY __attribute__((section(".entry")))



extern void _init();

extern uint8_t __text_start[];
extern uint8_t __data_start[];
extern uint8_t __rodata_start[];
extern uint8_t __bss_start[];
extern uint8_t __end[];

static BootParams bootParams;

static uint8_t kernel_stack[KERNEL_STACK_SIZE];

void crash_me();

void keyboard_handler(Registers* regs)
{
}

void ENTRY start(BootParams* bParams)
{
    // copy bootParams to kernel
    bootParams = *bParams;

    // initialize stack
    void *kernel_stack_end = &kernel_stack[KERNEL_STACK_SIZE];
    set_Stack((uint32_t)kernel_stack_end);

    // call global constructors
    _init();

    HAL_Initialize();
    memory_Initialize(&bootParams.Memory, bootParams.kernelSize);


    i686_IRQ_RegisterHandler(0, timer);

    //i686_IRQ_RegisterHandler(1, keyboard_handler);

    printf("Hello world from kernel!\n");

    log_debug("Main", "Boot device: 0x%x", bootParams.BootDevice);
    log_debug("Main", "Memory region count: 0x%x", bootParams.Memory.RegionCount);
    for (int i = 0; i < bootParams.Memory.RegionCount; i++)
    {
        log_debug("Main", "MEM: start=0x%llx, length=0x%llx, type=%u",
            bootParams.Memory.Regions[i].Begin,
            bootParams.Memory.Regions[i].Length,
            bootParams.Memory.Regions[i].Type);
    }

end:
    for (;;);
}