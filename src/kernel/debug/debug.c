#include "debug.h"
#include <arch/i686/io.h>

void dbg_putc(char c)
{
    i686_outb(0xE9, c);
}

void dbg_puts(const char* str)
{
    while (*str)
    {
        dbg_putc(*str);
        str++;
    }
}