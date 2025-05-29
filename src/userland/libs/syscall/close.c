#include "close.h"
#include "syscall.h"

int32_t close(int fd)
{
    return (int32_t)syscall(0x6, (uint32_t)fd, 0, 0, 0, 0);
}