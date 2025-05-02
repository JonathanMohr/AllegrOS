#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include "timer.h"
#include "keyboard/key.h"

#define KEY_EXTENDED 0xE0

extern uint8_t __bss_start;
extern uint8_t __end;

static bool extended = false;
static bool e1_sequence = false;
static int e1_index = 0;
static uint8_t e1_bytes[3];

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

    int result = handle_key(key);

    switch (result) {
        case 0:
            break;
        case 1:
            if (extended) {
                printf("Unknown extended Key: 0x%d\n", key);
            } else {
                printf("Unknown Key: 0x%d\n", key);
            }
        default:
            printf("Unknown result: %d\n", result);
            break;
    }

    extended = false;

    i686_outb(0x20, 0x20);
    i686_outb(0xA0, 0x20);
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HAL_Initialize();

    clrscr();

    printf("Hello world from kernel!\n");

    i686_IRQ_RegisterHandler(0, timer);

    i686_IRQ_RegisterHandler(1, keyboard);

end:
    for (;;);
}