#include "read.h"
#include "syscall.h"
#include <stddef.h>

int32_t read(int fd, void* buf, uint32_t count)
{
    return (int32_t)syscall(0x3, fd, (uint32_t)buf, count, 0, 0);
}