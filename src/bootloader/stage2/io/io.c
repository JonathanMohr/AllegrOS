#include "io.h"

#include <stdarg.h>
#include <stdbool.h>

#include "debug/debug.h"
#include "vga/vga.h"

void IO_Init()
{
    VGA_ClearScreen();
}

void IO_PutChar(stream_t s, char c)
{
    switch (s)
    {
        case vgaout:
            VGA_PutChar(c);
            break;

        case dbgout:
            Debug_PutChar(c);
            break;
    }
}

void IO_PutString(stream_t s, const char* str)
{
    while (*str)
    {
        IO_PutChar(s, *str);
        str++;
    }
}


const char g_HexChars[] = "0123456789abcdef";

void IO_PrintFormat_Unsigned(stream_t s, uint64_t number, int radix)
{
    char buffer[65];
    int pos = 0;

    // convert number to ASCII
    do 
    {
        uint64_t rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        IO_PutChar(s, buffer[pos]);
}

void IO_PrintFormat_Signed(stream_t s, int64_t number, int radix)
{
    if (number < 0)
    {
        IO_PutChar(s, '-');
        IO_PrintFormat_Unsigned(s, -number, radix);
    }
    else IO_PrintFormat_Unsigned(s, number, radix);
}

void IO_PrintFormat(stream_t s, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt)
    {
        switch (*fmt)
        {
            case '%':
            {
                fmt++;
                if(!*fmt) break;

                switch (*fmt)
                {
                    case 'u':
                    {
                        fmt++;
                        if(!*fmt) break;
                        char base = *fmt;

                        fmt++;
                        if(!*fmt) break;
                        char size = *fmt;

                        int radix;

                        switch (base)
                        {
                            case 'b': case 'B': radix = 2;  break;
                            case 'o': case 'O': radix = 8;  break;
                            case 'd': case 'D': radix = 10; break;
                            case 'h': case 'H':
                            case 'x': case 'X': radix = 16; break;

                            default:
                                fmt++;
                                continue;
                        }

                        switch (size)
                        {
                            case 'b': case 'B':
                            {
                                IO_PrintFormat_Unsigned(s, va_arg(args, unsigned int), radix);
                                break;
                            }

                            case 'w': case 'W':
                            {
                                IO_PrintFormat_Unsigned(s, va_arg(args, unsigned int), radix);
                                break;
                            }

                            case 'd': case 'D':
                            {
                                IO_PrintFormat_Unsigned(s, va_arg(args, uint32_t), radix);
                                break;
                            }

                            case 'q': case 'Q':
                            {
                                IO_PrintFormat_Unsigned(s, va_arg(args, uint64_t), radix);
                                break;
                            }
                        }

                        break;
                    }

                    case 'i':
                    {
                        fmt++;
                        if(!*fmt) break;
                        char base = *fmt;

                        fmt++;
                        if(!*fmt) break;
                        char size = *fmt;

                        int radix;

                        switch (base)
                        {
                            case 'b': case 'B': radix = 2;  break;
                            case 'o': case 'O': radix = 8;  break;
                            case 'd': case 'D': radix = 10; break;
                            case 'x': case 'X': radix = 16; break;

                            default:
                                fmt++;
                                continue;
                        }

                        switch (size)
                        {
                            case 'b': case 'B':
                            {
                                IO_PrintFormat_Signed(s, va_arg(args, int), radix);
                                break;
                            }

                            case 'w': case 'W':
                            {
                                IO_PrintFormat_Signed(s, va_arg(args, int), radix);
                                break;
                            }

                            case 'd': case 'D':
                            {
                                IO_PrintFormat_Signed(s, va_arg(args, int32_t), radix);
                                break;
                            }

                            case 'q': case 'Q':
                            {
                                IO_PrintFormat_Signed(s, va_arg(args, int64_t), radix);
                                break;
                            }
                        }

                        break;
                    }

                    case 'c':
                        IO_PutChar(s, (char)va_arg(args, int));
                        break;

                    case 's':
                        IO_PutString(s, va_arg(args, const char*));
                        break;

                    case '%':
                        IO_PutChar(s, '%');
                        break;
                }

                break;
            }

            default:
                IO_PutChar(s, *fmt);
                break;
        }

        fmt++;
    }
}
