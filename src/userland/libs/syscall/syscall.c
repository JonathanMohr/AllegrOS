#include "syscall.h"

#include <stddef.h>

extern uint32_t ASMCALL syscall(uint32_t eax,
                                uint32_t ebx,
                                uint32_t ecx,
                                uint32_t edx,
                                uint32_t esi,
                                uint32_t edi,
                                uint32_t ebp);

#define SYS_EXIT        1   // handled
#define SYS_FORK        2
#define SYS_READ        3   // handled
#define SYS_WRITE       4   // handled
#define SYS_OPEN        5   // handled
#define SYS_CLOSE       6   // handled
#define SYS_WAITPID     7
#define SYS_CREAT       8
#define SYS_LINK        9
#define SYS_UNLINK      10
#define SYS_EXECVE      11
#define SYS_CHDIR       12
#define SYS_TIME        13
#define SYS_MKNOD       14
#define SYS_CHMOD       15
#define SYS_LCHOWN      16
#define SYS_BREAK       17
#define SYS_OLDSTAT     18
#define SYS_LSEEK       19  // handled
#define SYS_GETPID      20
#define SYS_MOUNT       21
#define SYS_UNMOUNT     22
#define SYS_SETUID      23
#define SYS_GETUID      24
#define SYS_STIME       25
#define SYS_PTRACE      26
#define SYS_ALARM       27
#define SYS_OLDFSTAT    28
#define SYS_PAUSE       29
#define SYS_UTIME       30
//... TODO

void exit(int exit_code)
{
    syscall(SYS_EXIT, exit_code, 0, 0, 0, 0, 0);
}

int32_t read(int fd, void* buf, uint32_t count)
{
    return (int32_t)syscall(SYS_READ, fd, (uint32_t)buf, count, 0, 0, 0);
}

int32_t write(int fd, const void* buf, uint32_t count)
{
    return (int32_t)syscall(SYS_WRITE, fd, (uint32_t)buf, count, 0, 0, 0);
}

int32_t open(const char *pathname, int flags, uint16_t mode)
{
    return (int32_t)syscall(SYS_OPEN, (uint32_t)pathname, (uint32_t)flags, (uint32_t)mode, 0, 0, 0);
}

int32_t close(int32_t fd)
{
    return (int32_t)syscall(SYS_CLOSE, (uint32_t)fd, 0, 0, 0, 0, 0);
}

int64_t lseek(int32_t fd, int64_t offset, int whence)
{
    uint32_t offset_low = (uint32_t)(offset & 0xFFFFFFFF);
    uint32_t offset_high = (uint32_t)((offset >> 32) & 0xFFFFFFFF);
    return (uint64_t)syscall(SYS_LSEEK, (uint32_t)fd, offset_low, offset_high, (uint32_t)whence, 0, 0);
}