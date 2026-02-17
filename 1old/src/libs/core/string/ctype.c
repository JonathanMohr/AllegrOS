#include "ctype.h"

#include <stdbool.h>

bool islower_(char chr)
{
    return chr >= 'a' && chr <= 'z';
}

char toupper(char chr)
{
    return islower_(chr) ? (chr - 'a' + 'A') : chr;
}