#pragma once

#include <stdint.h>

void VGA_RawPutCharacter(uint8_t c, uint8_t x, uint8_t y);
void VGA_RawPutColor(uint8_t color, uint8_t x, uint8_t y);
uint8_t VGA_RawGetCharacter(uint8_t x, uint8_t y);
uint8_t VGA_RawGetColor(uint8_t x, uint8_t y);

void VGA_SetCursor(uint8_t x, uint8_t y);
void VGA_Scrollback(uint8_t lines);

void VGA_ClearScreen(void);
void VGA_PutChar(char c);
