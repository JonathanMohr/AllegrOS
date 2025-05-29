#include "stdio.h"
#include <syscall/write.h>
#include <string.h>

void putc(char c)
{
    fputc(c, STDOUT);
}

void puts(const char* str)
{
    fputs(str, STDOUT);
}