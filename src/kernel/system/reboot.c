#include "reboot.h"
#include "arch/i686/io.h"

#define KBD_CTRL_PORT 0x64
#define KBD_RESET_CMD 0xFE

void reboot_kbc(void) {
    while (i686_inb(KBD_CTRL_PORT) & 0x02);
    i686_outb(KBD_RESET_CMD, KBD_CTRL_PORT);
}

void reboot_cf9(void) {
    i686_outb(0x06, 0xCF9 & 0xFF);
    i686_outb(0x06, (0xCF9 >> 8) & 0xFF);
}

void reboot(void) {
    reboot_kbc();
    reboot_cf9();
    triple_fault();
}