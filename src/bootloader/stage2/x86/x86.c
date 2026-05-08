#include "x86.h"

#include <stddef.h>

uint32_t CDECL strlen(const char* str)
{
    uint32_t length = 0;
    while (str[length] != '\0') length++;
    return length;
}

char* CDECL strchr(const char* str, int32_t c)
{
    uint8_t ch = (uint8_t)c;

    while (*str != '\0')
    {
        if ((uint8_t)*str == ch)
            return (char*)str;
        str++;
    }

    if (ch == '\0') return (char*)str;

    return NULL;
}

int8_t CDECL toupper(int8_t c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}
