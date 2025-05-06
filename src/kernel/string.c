#include "string.h"
#include <stdint.h>
#include <stddef.h>

const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strcpy(char* dst, const char* src)
{
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}

int strcmp(const char* a, const char* b)
{
    if (a == NULL && b == NULL)
        return 0;

    if (a == NULL || b == NULL)
        return -1;

    while (*a && *b && *a == *b)
    {
        ++a;
        ++b;
    }
    return (*a) - (*b);
}

size_t strcspn(const char *str1, const char *str2)
{
    const char *s1 = str1;
    const char *s2;
    
    while (*s1) {
        for (s2 = str2; *s2; ++s2) {
            if (*s1 == *s2) {
                return s1 - str1;
            }
        }
        ++s1;
    }
    
    return s1 - str1;
}