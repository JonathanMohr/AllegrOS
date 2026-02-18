#include <abi.h>
#include <stdint.h>

#include "vga/vga.h"

void CDECL start(uint16_t boot_drive)
{
    VGA_ClearScreen();

    VGA_PutString("Hello World from C!\n");

    for (;;);
}
