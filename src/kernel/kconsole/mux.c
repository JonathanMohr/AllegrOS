#include "kconsole.h"

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
