#include "syscall.h"

#include "debug.h"

void syscall(Registers* regs)
{
    log_info("SYSCALL", "int 0x80 from userland! eax=%x", regs->eax);
}