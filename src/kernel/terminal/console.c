#include "console.h"
#include "arch/i686/io.h"

void putcolor(int x, int y, uint8_t color)
{
    VGA_putcolor(x, y, color);
}

char getchr(int x, int y)
{
    return VGA_getchr(x, y);
}

uint8_t getcolor(int x, int y)
{
    return getcolor(x, y);
}

void setcursor(int x, int y)
{
    VGA_setcursor(x, y);
}

void clrscr()
{
    VGA_clrscr();
}

void scrollback(int lines)
{
    VGA_scrollback(lines);
}