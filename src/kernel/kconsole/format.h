#pragma once

#include "kconsole.h"
#include <stdarg.h>

void KernelConsole_PrintFormatV(KernelConsole* kconsole, const char* fmt, va_list args);
void KernelConsole_PrintFormat(KernelConsole* kconsole, const char* fmt, ...);
