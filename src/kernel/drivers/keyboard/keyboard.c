#include "keyboard.h"
#include "../../arch/i686/io.h"
#include "../../debug.h"
#include "scancode.h"
#include <stdbool.h>
#include "keys.h"
#include "key.h"

static bool extended = false;
static bool e1_sequence = false;
static int e1_index = 0;
static uint8_t e1_bytes[3];

#define KEY_EXTENDED 0xE0

void keyboard_handler(Registers* regs)
{
    //TODO
    uint8_t scancode = i686_inb(0x60);
    //TODO: remove log
    log_verbose("Scancode", "0x%x", scancode);

    if (scancode == 0xE1 && !e1_sequence) {
        e1_sequence = true;
        e1_index = 0;
        return;
    }

    if (e1_sequence) {
        if (e1_index < 3) {
            e1_bytes[e1_index++] = scancode;
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
    
    if (scancode == KEY_EXTENDED && !extended) {
        extended = true;
        return;
    }

    uint16_t key = get_key(scancode, extended);

    extended = false;

    bool released = (key & KEY_RELEASED) != 0;
    key = key & ~KEY_RELEASED;

    InputEvent event;
    event.type = EV_KEY;
    event.code = key;
    event.value = released ? 0 : 1;
    InputBuffer_Push(&event);

    //TODO: check if needed by anything
    i686_outb(0x20, 0x20);
    i686_outb(0xA0, 0x20);
}

InputEvent inputBuffer[INPUT_EVENT_BUFFER_SIZE];
static uint32_t input_head = 0;
static uint32_t input_tail = 0;


// Wird vom Input-Treiber aufgerufen (z.B. Keyboard-ISR)
void InputBuffer_Push(InputEvent* event)
{
    uint32_t next_head = (input_head + 1) % INPUT_EVENT_BUFFER_SIZE;
    if (next_head != input_tail)
    {
        inputBuffer[input_head] = *event;
        input_head = next_head;
    }
}

// Liest Events aus dem Puffer
uint32_t InputBuffer_Read(InputEvent* out_events, uint32_t max_events)
{
    uint32_t count = 0;
    while (count < max_events && input_tail != input_head)
    {
        out_events[count++] = inputBuffer[input_tail];
        input_tail = (input_tail + 1) % INPUT_EVENT_BUFFER_SIZE;
    }
    return count;
}