#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include "timer.h"
#include "debug.h"

extern void _init();

static uint64_t current_program = UINT64_MAX;

void crash_me();

void keyboard_handler(Registers* regs)
{
}

void start(uint16_t bootDrive)
{
    // call global constructors
    _init();

    HAL_Initialize();

    i686_IRQ_RegisterHandler(0, timer);

    //i686_IRQ_RegisterHandler(1, keyboard_handler);

    /*
    log_verbose("Main", "This is a verbose msg!");
    log_debug("Main", "This is a debug msg!");
    log_info("Main", "This is an info msg!");
    log_warn("Main", "This is a warning msg!");
    log_err("Main", "This is an error msg!");
    log_crit("Main", "This is a critical msg!");
    */

    printf("Hello world from kernel!\n");

end:
    for (;;);
}