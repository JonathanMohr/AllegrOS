#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "hal/hal.h"
#include "timer.h"
#include "debug.h"
#include <boot/bootparams.h>
#include <core/memory/memory.h>
#include "memory/memory.h"
#include "hal/paging.h"

#define ENTRY __attribute__((section(".entry")))

extern void _init();

extern uint8_t __text_start[];
extern uint8_t __data_start[];
extern uint8_t __rodata_start[];
extern uint8_t __bss_start[];
extern uint8_t __end[];

static uint64_t current_program = UINT64_MAX;

void crash_me();

void keyboard_handler(Registers* regs)
{
}

void ENTRY start(BootParams* bootParams)
{
    // call global constructors
    _init();

    HAL_Initialize();

    i686_IRQ_RegisterHandler(0, timer);

    //i686_IRQ_RegisterHandler(1, keyboard_handler);

    printf("Hello world from kernel!\n");

    memory_Initialize(&bootParams->Memory, (uintptr_t)__text_start, (uintptr_t)__end);

    Paging_Initialize((uintptr_t)__end);

    log_debug("Main", "Boot device: %x", bootParams->BootDevice);
    log_debug("Main", "Memory region count: %x", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++)
    {
        log_debug("Main", "MEM: start=0x%llx, length=0x%llx, type=%x",
            bootParams->Memory.Regions[i].Begin,
            bootParams->Memory.Regions[i].Length,
            bootParams->Memory.Regions[i].Type);
    }

    memory_Initialize_Allocator();

    char* test = memory_Allocate(sizeof(char), 1);
    
    if(test)
    {
        memset(test, 'a', 1);
        log_debug("", "Test: %c", *test);
    }

end:
    for (;;);
}