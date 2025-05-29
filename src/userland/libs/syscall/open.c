#include "open.h"
#include "syscall.h"

int32_t open(const char *pathname, int flags, uint16_t mode)
{
    return (int32_t)syscall(0x5, (uint32_t)pathname, (uint32_t)flags, (uint32_t)mode, 0, 0);
}