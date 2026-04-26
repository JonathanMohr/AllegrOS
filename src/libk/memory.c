#include "memory.h"

int CDECL memcmp(const char* a, const char* b, size_t n)
{
    int diff = 0;
    for (size_t i = 0; i < n && diff == 0; i++)
    {
        diff = (int)a[i] - (int)b[i];
    }
    return diff;
}
