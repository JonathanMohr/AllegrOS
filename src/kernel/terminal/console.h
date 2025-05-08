#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../arch/i686/vga_text.h"

void putcolor(int x, int y, uint8_t color);
char getchr(int x, int y);
uint8_t getcolor(int x, int y);
void setcursor(int x, int y);
void clrscr();
void scrollback(int lines);