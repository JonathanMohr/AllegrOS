#include "syscall.h"

#include "../debug.h"
#include <core/Defs.h>

#include "file.h"

extern void set_eax(uint32_t val);
extern void set_edx(uint32_t val);

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
//... TODO: https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86-32_bit


void syscall_Init(Partition* part)
{
    File_Init(part);
}


void syscall(Registers* regs)
{
    uint32_t syscall = regs->eax;
    switch (syscall)
    {
        case SYS_EXIT:
            int code = regs->ebx;
            //TODO
            for(;;);
            break;
        
        case SYS_READ:
            int32_t read = File_Read(regs->ebx, (uint8_t*)regs->ecx, regs->edx);
            set_eax(read);
            break;
        
        case SYS_WRITE:
            int32_t write = File_Write(regs->ebx, (uint8_t*)regs->ecx, regs->edx);
            set_eax(write);
            break;
        
        case SYS_OPEN:
            const char* path = (const char*)regs->ebx;
            int32_t fd = File_Open(path, regs->ecx, regs->edx);
            set_eax(fd);
            break;
        
        case SYS_CLOSE:
            int32_t result = File_Close(regs->ebx);
            set_eax(result);
            break;

        case SYS_LSEEK:
            int64_t offset = ((int64_t)(uint32_t)regs->edx << 32) | (uint32_t)regs->ecx;
            int64_t lseek = File_Seek(regs->ebx, offset, regs->esi);

            uint32_t low = (uint32_t)(lseek & 0xFFFFFFFF);
            uint32_t high = (uint32_t)((lseek >> 32) & 0xFFFFFFFF);

            set_eax(low);
            set_edx(high);
            break;
        
        case 0:
        default:
            log_warn("SYSCALL", "Unknown system call: %lu", syscall);
            log_warn("SYSCALL", "ebx: 0x%lx, ecx: 0x%lx, edx: 0x%lx", regs->ebx, regs->ecx, regs->edx);
            log_warn("SYSCALL", "esi: 0x%lx, edi: 0x%lx, ebp: 0x%lx", regs->esi, regs->edi, regs->ebp);
            set_eax(0);
            break;
    }
}