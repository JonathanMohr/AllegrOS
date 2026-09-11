#include "format.h"

#include "../memory/result.h"

static const char hexChars[] = "0123456789abcdef";

static void PrintUnsigned(KernelConsole* kconsole, uint64_t number, uint8_t radix)
{
    char buffer[65];
    int32_t pos = 0;

    // convert number to ASCII
    do
    {
        uint64_t rem = number % (uint64_t)radix;
        number /= (uint64_t)radix;
        buffer[pos++] = hexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        KernelConsole_PutChar(kconsole, buffer[pos]);
}

static void PrintSigned(KernelConsole* kconsole, int64_t number, uint8_t radix)
{
    uint64_t u = (uint64_t)number;
    if (number < 0)
    {
        KernelConsole_PutChar(kconsole, '-');
        u = -u;
    }

    PrintUnsigned(kconsole, u, radix);
}

void KernelConsole_PrintFormatV(KernelConsole* kconsole, const char* fmt, va_list args)
{
    while (*fmt)
    {
        switch (*fmt)
        {
            case '%':
            {
                fmt++;
                if (!*fmt) break;

                switch (*fmt)
                {
                    case 'u': case 'U': // Unsigned integer
                    case 'i': case 'I': // Signed integer
                    {
                        int isUnsigned = 0;
                        if (*fmt == 'u' || *fmt == 'U') isUnsigned = 1;

                        fmt++;
                        if (!*fmt) break;
                        char base = *fmt;

                        fmt++;
                        if (!*fmt) break;
                        char size = *fmt;

                        uint8_t radix = 10; // Default
                        switch (base)
                        {
                            case 'b': case 'B':
                                radix = 2;
                                break;

                            case 'o': case 'O':
                                radix = 8;
                                break;

                            case 'd': case 'D':
                                radix = 10;
                                break;

                            case 'h': case 'H':
                            case 'x': case 'X':
                                radix = 16;
                                break;
                        }

                        switch (size)
                        {
                            case 'b': case 'B': // 8-bit
                                if (isUnsigned != 0)
                                    PrintUnsigned(kconsole, (uint8_t)va_arg(args, unsigned int), radix);
                                else
                                    PrintSigned(kconsole, (int8_t)va_arg(args, int), radix);
                                break;

                            case 'w': case 'W': // 16-bit
                                if (isUnsigned != 0)
                                    PrintUnsigned(kconsole, (uint16_t)va_arg(args, unsigned int), radix);
                                else
                                    PrintSigned(kconsole, (int16_t)va_arg(args, int), radix);
                                break;

                            case 'd': case 'D': // 32-bit
                                if (isUnsigned != 0)
                                    PrintUnsigned(kconsole, va_arg(args, uint32_t), radix);
                                else
                                    PrintSigned(kconsole, va_arg(args, int32_t), radix);
                                break;

                            case 'q': case 'Q': // 64-bit
                                if (isUnsigned != 0)
                                    PrintUnsigned(kconsole, va_arg(args, uint64_t), radix);
                                else
                                    PrintSigned(kconsole, va_arg(args, int64_t), radix);
                                break;
                        }

                        break;
                    }

                    case 'r': case 'R':
                    {
                        fmt++;
                        if (!*fmt) break;
                        const char resultType = *fmt;

                        switch (resultType)
                        {
                            case 'm': case 'M':
                                PrintUnsigned(kconsole, (Memory_Result)va_arg(args, Memory_Result_VaArg), 10);
                                break;
                        }

                        break;
                    }

                    case 'p': case 'P':
                        PrintUnsigned(kconsole, (uintptr_t)va_arg(args, uintptr_t), 16);
                        break;

                    case 'q': case 'Q':
                        PrintUnsigned(kconsole, (uphysptr_t)va_arg(args, uphysptr_t), 16);
                        break;

                    case 'c': case 'C':
                        KernelConsole_PutChar(kconsole, (char)va_arg(args, int));
                        break;

                    case 's': case 'S':
                        KernelConsole_PutString(kconsole, va_arg(args, const char*));
                        break;
                        
                    case '%':
                        KernelConsole_PutChar(kconsole, '%');
                        break;
                }

                break;
            }

            default:
                KernelConsole_PutChar(kconsole, *fmt);
                break;
        }

        if (!*fmt) break;
        fmt++;
    }
}

void KernelConsole_PrintFormat(KernelConsole* kconsole, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    KernelConsole_PrintFormatV(kconsole, fmt, args);

    va_end(args);
}
