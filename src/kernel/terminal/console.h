#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdbool.h>

extern const unsigned SCREEN_WIDTH;
extern const unsigned SCREEN_HEIGHT;
extern uint8_t DEFAULT_COLOR;

extern uint8_t* g_ScreenBuffer;
extern int g_ScreenX, g_ScreenY;

void putchr(int x, int y, char c);
void putcolor(int x, int y, uint8_t color);
char getchr(int x, int y);
uint8_t getcolor(int x, int y);
void setcursor(int x, int y);
void clrscr(uint8_t color);
void scrollback(int lines);
void putc(char c);
void puts(const char* str);

#endif