#include "debug.h"
#include <stdarg.h>

static const char* const g_LogSeverityColors[] =
{
    [LVL_VERBOSE]      = "\033[0;36m",
    [LVL_DEBUG]        = "\033[2;37m",
    [LVL_INFO]         = "\033[37m",
    [LVL_WARN]         = "\033[1;33m",
    [LVL_ERROR]        = "\033[1;31m",
    [LVL_CRITICAL]     = "\033[1;37;41m",
};

static const char* const g_ColorReset = "\033[0m";

void logf(const char* module, DebugLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (level < MIN_LOG_LEVEL)
        return;
}