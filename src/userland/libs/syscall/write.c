#include "write.h"
#include <string.h>
#include "syscall.h"

int32_t write(int fd, const void* buf, uint32_t count)
{
    return (int32_t)syscall(0x4, fd, (uint32_t)buf, count, 0, 0);
}