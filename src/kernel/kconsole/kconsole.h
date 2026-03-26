#pragma once

#include <stdint.h>

typedef struct KernelConsole {
    void (*putChar)(void* context, char c);
    void (*clear)(void* context);
    void* context;
} KernelConsole;

KernelConsole* KernelConsole_GetVGA();
KernelConsole* KernelConsole_GetDebug();
KernelConsole* KernelConsole_GetOutput();

void KernelConsole_ClearScreen(KernelConsole* kconsole);
void KernelConsole_PutChar(KernelConsole* kconsole, char c);
void KernelConsole_PutString(KernelConsole* kconsole, const char* str);

typedef struct KernelConsoleMux {
    KernelConsole** consoles;
    uint32_t count;
    KernelConsole console;
} KernelConsoleMux;

KernelConsole* KernelConsole_InitMux(KernelConsoleMux* mux, KernelConsole** consoles, uint32_t count);
