#include "string.h"

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