#include "vga.h"

#include "../../x86/x86.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
const uint8_t DEFAULT_COLOR = 0x7;

uint16_t* vga_buffer = (uint16_t*)0xB8000;
uint8_t vga_screenX = 0;
uint8_t vga_screenY = 0;

void VGA_RawPutCharacter(uint8_t c, uint8_t x, uint8_t y)
{
    vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0xFF00) | (c & 0xFF);
}

void VGA_RawPutColor(uint8_t color, uint8_t x, uint8_t y)
{
    vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (uint16_t)(color << 8) | (vga_buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0x00FF);
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
    uint16_t cursorPosition = ((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x;

    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(cursorPosition & 0xFF));

    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((cursorPosition >> 8) & 0xFF));
}

void VGA_Scrollback(uint8_t lines)
{
    for (uint8_t y = lines; y < SCREEN_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            VGA_RawPutCharacter(VGA_RawGetCharacter(x, y), x, y - lines);
            VGA_RawPutColor(VGA_RawGetColor(x, y), x, y - lines);
        }
    }

    for (uint8_t y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            VGA_RawPutCharacter((uint8_t)'\0', x, y);
            VGA_RawPutColor(DEFAULT_COLOR, x, y);
        }
    }

    vga_screenY -= lines;
}


void VGA_ClearScreen(void)
{
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            VGA_RawPutCharacter((uint8_t)'\0', x, y);
            VGA_RawPutColor(DEFAULT_COLOR, x, y);
        }
    }

    vga_screenX = 0;
    vga_screenY = 0;
    VGA_SetCursor(vga_screenX, vga_screenY);
}

void VGA_PutChar(char c)
{
    switch (c)
    {
        case '\n':
            vga_screenX = 0;
            vga_screenY++;
            break;

        case '\t':
            for (uint8_t i = 0; i < 4 - (vga_screenX % 4); i++)
                VGA_PutChar((uint8_t)' ');
            break;

        case '\r':
            vga_screenX = 0;
            break;

        default:
            VGA_RawPutCharacter((uint8_t)c, vga_screenX, vga_screenY);
            vga_screenX++;
            break;
    }

    if (vga_screenX >= SCREEN_WIDTH)
    {
        vga_screenY++;
        vga_screenX = 0;
    }
    if (vga_screenY >= SCREEN_HEIGHT)
        VGA_Scrollback(1);

    VGA_SetCursor(vga_screenX, vga_screenY);
}
