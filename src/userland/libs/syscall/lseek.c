#include "lseek.h"
#include "syscall.h"

int64_t lseek(int32_t fd, int64_t offset, int whence)
{
    uint32_t offset_low = (uint32_t)(offset & 0xFFFFFFFF);
    uint32_t offset_high = (uint32_t)((offset >> 32) & 0xFFFFFFFF);
    return (uint64_t)syscall(0x8, (uint32_t)fd, offset_low, offset_high, (uint32_t)whence, 0);
}