#include "vga.h"

#include "../x86/x86.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
const uint8_t DEFAULT_COLOR = 0x7;

uint16_t* vga_buffer = (uint16_t*)0xB8000;
uint8_t vga_screenX = 0;
uint8_t vga_screenY = 0;

void VGA_RawPutChar(uint8_t c, uint8_t x, uint8_t y)
{
    vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0xFF00) | (c & 0xFF);
}

void VGA_RawPutColor(uint8_t color, uint8_t x, uint8_t y)
{
    vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (color << 8) | (vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0x00FF);
}

uint8_t VGA_RawGetCharacter(uint8_t x, uint8_t y)
{
    return vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0x00FF;
}

uint8_t VGA_RawGetColor(uint8_t x, uint8_t y)
{
    return vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] >> 8;
}

void VGA_SetCursor(uint8_t x, uint8_t y)
{
    uint16_t cursorPosition = ((uint16_t)y * SCREEN_WIDTH) + x;

    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(cursorPosition & 0xFF));

    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((cursorPosition >> 8) & 0xFF));
}


void VGA_ClearScreen()
{
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            VGA_RawPutChar('\0', x, y);
            VGA_RawPutColor(DEFAULT_COLOR, x, y);
        }
    }

    vga_screenX = 0;
    vga_screenY = 0;
    VGA_SetCursor(vga_screenX, vga_screenY);
}
