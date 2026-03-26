#include <bootparams.h>
#include <abi.h>

#include "panic/panic.h"

#include "kconsole/kconsole.h"
#include "kconsole/format.h"

#include "memory/memory.h"
#include "memory/physical/manager.h"
#include "memory/virtual/manager.h"

#include "arch/x86/x86.h"

void CDECL kmain(BootParams* bParams)
{
    BootParams bootParams = *bParams;

    x86_Initialize();

    if (!Memory_Initialize(&bootParams.Memory))
    {
        PanicMessage("[KERNEL] Could not initialize memory\n");
        Panic();
    }

    KernelConsole* mux_console = KernelConsole_GetOutput();

    KernelConsole_ClearScreen(mux_console);
    KernelConsole_PutString(mux_console, "Hello world from kernel!\n");

    __asm__("sti");

end:
    for(;;);
}
