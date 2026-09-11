#include "vga.h"

#include <stdint.h>

#include "../../arch/x86/x86.h"
#include "../../memory/physical.h"
#include "../../memory/memory.h"
#include "../../panic/panic.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
const static uint8_t DEFAULT_COLOR = 0x7;

static uint16_t* VGA_Buffer = NULL;
static uint8_t VGA_screenX = 0;
static uint8_t VGA_screenY = 0;

static void VGA_RawPutCharacter(uint8_t c, uint8_t x, uint8_t y)
{
    VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0xFF00) | (c & 0xFF);
}

static void VGA_RawPutColor(uint8_t color, uint8_t x, uint8_t y)
{
    VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] =
        (uint16_t)(color << 8) | (VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0x00FF);
}

static uint8_t VGA_RawGetCharacter(uint8_t x, uint8_t y)
{
    return VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] & 0x00FF;
}

static uint8_t VGA_RawGetColor(uint8_t x, uint8_t y)
{
    return VGA_Buffer[((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x] >> 8;
}

static void VGA_SetCursor(uint8_t x, uint8_t y)
{
    uint16_t cursorPosition = ((uint16_t)y * SCREEN_WIDTH) + (uint16_t)x;

    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(cursorPosition & 0xFF));

    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((cursorPosition >> 8) & 0xFF));
}

static void VGA_Scrollback(uint8_t lines)
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

    VGA_screenY -= lines;
}

bool VGA_Initialize(void)
{
    const uphysptr_t start = 0xB8000;
    const uphysptr_t end = start + 2 * SCREEN_WIDTH * SCREEN_HEIGHT;
    
    const uphysptr_t startPage = start / MEMORY_PAGE_SIZE;
    const uphysptr_t endPage = (end + MEMORY_PAGE_SIZE - 1) / MEMORY_PAGE_SIZE;
    const uphysptr_t pageCount = endPage - startPage;

    if (Memory_Physical_Reserve(startPage * MEMORY_PAGE_SIZE, pageCount) != MEMORY_SUCCESS)
        return false;

    uintptr_t virtualAddress;
    if (Memory_Kernel_AllocateVirtual(pageCount, &virtualAddress) != MEMORY_SUCCESS)
    {
        if (Memory_Physical_Unreserve(startPage * MEMORY_PAGE_SIZE, pageCount) != MEMORY_SUCCESS)
            PanicMessageInfo("VGA_Initialize", "Memory_Physical_Unreserve(%q, %q) failed\n", startPage * MEMORY_PAGE_SIZE, pageCount);
        return false;
    }

    for (uphysptr_t i = 0; i < pageCount; i++)
    {
        if (Memory_LinkRaw(MEMORY_KERNEL, virtualAddress + i * MEMORY_PAGE_SIZE, startPage * MEMORY_PAGE_SIZE + i * MEMORY_PAGE_SIZE, MEMORY_WRITABLE) != MEMORY_SUCCESS)
        {
            if (i > 0 && Memory_Unlink(MEMORY_KERNEL, virtualAddress, i) != MEMORY_SUCCESS)
            {
                PanicMessageInfo("VGA_Initialize", "Memory_Unlink(MEMORY_KERNEL, %p, %p) failed\n", virtualAddress, i);
                return false;
            }

            if (Memory_Kernel_FreeVirtual(virtualAddress, pageCount) != MEMORY_SUCCESS)
                PanicMessageInfo("VGA_Initialize", "Memory_Kernel_FreeVirtual(MEMORY_KERNEL, %p, %p) failed\n", virtualAddress, pageCount);

            if (Memory_Physical_Unreserve(startPage * MEMORY_PAGE_SIZE, pageCount) != MEMORY_SUCCESS)
                PanicMessageInfo("VGA_Initialize", "Memory_Physical_Unreserve(%q, %q) failed\n", startPage * MEMORY_PAGE_SIZE, pageCount);

            return false;
        }
    }

    VGA_Buffer = (uint16_t*)(virtualAddress + start % MEMORY_PAGE_SIZE);

    return true;
}

void VGA_ClearScreen(void* context)
{
    if (!VGA_Buffer) return;

    (void)context;

    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            VGA_RawPutCharacter((uint8_t)'\0', x, y);
            VGA_RawPutColor(DEFAULT_COLOR, x, y);
        }
    }

    VGA_screenX = 0;
    VGA_screenY = 0;
    VGA_SetCursor(VGA_screenX, VGA_screenY);
}

void VGA_PutChar(void* context, char c)
{
    if (!VGA_Buffer) return;

    switch (c)
    {
        case '\n':
            VGA_screenX = 0;
            VGA_screenY++;
            break;

        case '\t':
            for (uint8_t i = 0; i < 4 - (VGA_screenX % 4); i++)
                VGA_PutChar(context, (uint8_t)' ');
            break;

        case '\r':
            VGA_screenX = 0;
            break;

        default:
            VGA_RawPutCharacter((uint8_t)c, VGA_screenX, VGA_screenY);
            VGA_screenX++;
            break;
    }

    if (VGA_screenX >= SCREEN_WIDTH)
    {
        VGA_screenY++;
        VGA_screenX = 0;
    }
    if (VGA_screenY >= SCREEN_HEIGHT)
        VGA_Scrollback(1);

    VGA_SetCursor(VGA_screenX, VGA_screenY);
}
