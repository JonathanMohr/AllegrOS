#include "syscall.h"

#include "../debug.h"

void syscall(Registers* regs)
{
    log_info("SYSCALL", "int 0x80 from userland! eax=0x%x", regs->eax);
    log_info("SYSCALL", "ebx=0x%x, ecx=0x%x, edx=0x%x, esi=0x%x, edi=0x%x",
             regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
}