#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include "timer.h"
#include "keyboard/key.h"
#include "program.h"
#include "debug/debug.h"

#define KEY_EXTENDED 0xE0

extern void _init();

static bool extended = false;
static bool e1_sequence = false;
static int e1_index = 0;
static uint8_t e1_bytes[3];


static uint64_t current_program = UINT64_MAX;


void crash_me();

void keyboard(Registers* regs)
{
    uint8_t keycode = i686_inb(0x60);

    if (keycode == 0xE1 && !e1_sequence) {
        e1_sequence = true;
        e1_index = 0;
        return;
    }

    if (e1_sequence) {
        if (e1_index < 3) {
            e1_bytes[e1_index++] = keycode;
            if (e1_index == 3) {
                e1_sequence = false;
                e1_index = 0;
                return;
            }
        } else {
            e1_sequence = false;
            e1_index = 0;
            return;
        }
    }
    
    if (keycode == KEY_EXTENDED && !extended) {
        extended = true;
        return;
    }

    uint16_t key = get_key(keycode, extended);

    int result = 1;

    if (current_program != UINT64_MAX) {
        result = handle_input(current_program, key);
    }

    switch (result) {
        case 0:
            break;
        case 1:
            if (extended) {
                printf("Unhandled extended Key: 0x%d\n", key);
            } else {
                printf("Unhandled Key: 0x%d\n", key);
            }
        default:
            printf("Unknown result: %d\n", result);
            break;
    }

    extended = false;

    i686_outb(0x20, 0x20);
    i686_outb(0xA0, 0x20);
}

void start(uint16_t bootDrive)
{
    // call global constructors
    _init();

    HAL_Initialize();

    current_program = program_init();

    i686_IRQ_RegisterHandler(0, timer);

    i686_IRQ_RegisterHandler(1, keyboard);

    dbg_puts("\033[1;31mHello world from my OS!\033[0m");

end:
    for (;;);
}