#include "keyboard.h"

#include <stddef.h>

#include "../../arch/i686/io.h"
#include "keys.h"
#include "key.h"

#include "../../stdio.h"

#define KEY_EXTENDED 0xE0

int keyboard(uint64_t current_program, KeyboardHandler handler, Registers* regs)
{
    uint8_t keycode = i686_inb(0x60);

    if (keycode == 0xE1 && !e1_sequence) {
        e1_sequence = true;
        e1_index = 0;
        return 0;
    }

    if (e1_sequence) {
        if (e1_index < 3) {
            e1_bytes[e1_index++] = keycode;
            if (e1_index == 3) {
                e1_sequence = false;
                e1_index = 0;
                return 0;
            }
        } else {
            e1_sequence = false;
            e1_index = 0;
            return 0;
        }
    }
    
    if (keycode == KEY_EXTENDED && !extended) {
        extended = true;
        return 0;
    }

    uint16_t key = get_key(keycode, extended);

    int result = -1;

    if (handler != NULL) {
        result = handler(current_program, key);
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

    return result;
}