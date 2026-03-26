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

static void KernelConsoleMux_ClearScreen(void* context)
{
    KernelConsoleMux* mux = (KernelConsoleMux*)context;
    for (uint32_t i = 0; i < mux->count; i++)
        mux->consoles[i]->clear(mux->consoles[i]->context);
}

static void KernelConsoleMux_PutChar(void* context, char c)
{
    KernelConsoleMux* mux = (KernelConsoleMux*)context;
    for (uint32_t i = 0; i < mux->count; i++)
        mux->consoles[i]->putChar(mux->consoles[i]->context, c);
}

KernelConsole* KernelConsole_InitMux(KernelConsoleMux* mux, KernelConsole** consoles, uint32_t count)
{
    mux->consoles = consoles;
    mux->count = count;
    mux->console.putChar = KernelConsoleMux_PutChar;
    mux->console.clear = KernelConsoleMux_ClearScreen;
    mux->console.context = mux;
    return &mux->console;
}

static KernelConsole* koutputConsoles[] = {
    &vgaConsole,
    &debugConsole
};

static KernelConsoleMux koutputConsole = {
    .consoles = koutputConsoles,
    .count = 2,
    .console.clear = KernelConsoleMux_ClearScreen,
    .console.putChar = KernelConsoleMux_PutChar,
    .console.context = &koutputConsole
};

KernelConsole* KernelConsole_GetOutput()
{
    return &koutputConsole.console;
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
