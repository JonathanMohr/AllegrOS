#pragma once

#include <stdint.h>

void VGA_init(uint8_t* vga_addr);
void VGA_clrscr();
void VGA_putc(char c);
void VGA_putcolor(int x, int y, uint8_t color);
char VGA_getchr(int x, int y);
uint8_t VGA_getcolor(int x, int y);
void VGA_setcursor(int x, int y);
void VGA_scrollback(int lines);
void VGA_puts(const char* str);