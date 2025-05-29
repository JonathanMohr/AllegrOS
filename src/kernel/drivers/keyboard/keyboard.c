#include "keyboard.h"
#include "../../arch/i686/io.h"
#include "../../debug.h"
#include "scancode.h"

void keyboard_handler(Registers* regs)
{
    //TODO
    uint8_t scancode = i686_inb(0x60);
    log_err("Kernel", "Key pressed: 0x%x", scancode);
}