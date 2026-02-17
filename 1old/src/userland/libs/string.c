#include "string.h"
#include <stddef.h>

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