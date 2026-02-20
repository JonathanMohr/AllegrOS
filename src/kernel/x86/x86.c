#include "x86.h"

#include <stddef.h>

int32_t CDECL memcmp(const char* a, const char* b, uint64_t n)
{
    int32_t diff = 0;
    for (uint64_t i = 0; i < n && diff == 0; i++)
    {
        diff = (int32_t)a[i] - (int32_t)b[i];
    }
    return diff;
}

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
