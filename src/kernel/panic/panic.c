#include "panic.h"

#include "../kconsole/kconsole.h"
#include "../kconsole/format.h"

void PanicMessage(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    KernelConsole_PrintFormatV(KernelConsole_GetDebug(), fmt, args);

    va_end(args);
}
