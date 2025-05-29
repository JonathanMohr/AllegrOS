#include "write.h"
#include <string.h>
#include "syscall.h"

int32_t write(int fd, const void* buf, uint32_t count)
{
    return (int32_t)syscall(0x4, fd, (uint32_t)buf, count, 0, 0);
}

void fputc(char c, int file)
{
    //TODO: check result
    write(file, &c, 1);
}

void fputs(const char* str, int file)
{
    //TODO: check result
    write(file, str, (uint32_t)strlen(str));
}