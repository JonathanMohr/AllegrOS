#include <bootparams.h>

#include "kconsole/kconsole.h"

void kmain(BootParams* bootParams)
{
    KernelConsole* vga_console = KernelConsole_GetVGA();
    KernelConsole* debug_console = KernelConsole_GetDebug();
    KernelConsole* kconsoles[2] = { vga_console, debug_console };
    KernelConsoleMux mux_kconsole;
    KernelConsole* mux_console = KernelConsole_InitMux(&mux_kconsole, kconsoles, sizeof(kconsoles) / sizeof(KernelConsole*));

    KernelConsole_ClearScreen(mux_console);
    KernelConsole_PutString(mux_console, "Hello world from kernel!\n");

end:
    for(;;);
}
