#include "keyboard.h"
#include "../arch/i686/io.h"
#include "../debug.h"

#define KBD_BUFFER_SIZE 128

static char kbd_buffer[KBD_BUFFER_SIZE];
static uint32_t kbd_buf_head = 0;
static uint32_t kbd_buf_tail = 0;

void keyboard_handler(Registers* regs)
{
    uint8_t scancode = i686_inb(0x60);  // Beispiel: PS/2 Scancode holen
    char c = translate_scancode(scancode);  // ASCII-Zeichen

    if (c && ((kbd_buf_tail + 1) % KBD_BUFFER_SIZE != kbd_buf_head)) {
        kbd_buffer[kbd_buf_tail] = c;
        kbd_buf_tail = (kbd_buf_tail + 1) % KBD_BUFFER_SIZE;
    }

    log_err("Kernel", "Key pressed: %c", c);
}