#include "kconsole.h"

#include "vga/vga.h"
#include "debug/debug.h"

KernelConsole vgaConsole = { .clear=VGA_ClearScreen, .putChar=VGA_PutChar };
KernelConsole debugConsole = { .clear=Debug_ClearScreen, .putChar=Debug_PutChar };

KernelConsole* KernelConsole_GetVGA()
{
    return &vgaConsole;
}

KernelConsole* KernelConsole_GetDebug()
{
    return &debugConsole;
}

void KernelConsole_ClearScreen(KernelConsole* kconsole)
{
    kconsole->clear(kconsole->context);
}

void KernelConsole_PutChar(KernelConsole* kconsole, char c)
{
    kconsole->putChar(kconsole->context, c);
}

void KernelConsole_PutString(KernelConsole* kconsole, const char* str)
{
    while (*str)
    {
        KernelConsole_PutChar(kconsole, *str);
        str++;
    }
}
