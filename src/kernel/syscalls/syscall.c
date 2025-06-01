#include "syscall.h"

#include "../debug.h"
#include <core/Defs.h>

#include "file.h"
#include "../hal/vfs.h"

extern void set_eax(uint32_t val);
extern void set_edx(uint32_t val);

#define EXIT    0x1
#define FORK    0x2     //TODO
#define READ    0x3
#define WRITE   0x4
#define OPEN    0x5
#define CLOSE   0x6
#define UNLINK  0x7     //TODO
#define LSEEK   0x8

void syscall_Init(Partition* part)
{
    File_Init(part);
}


void syscall(Registers* regs)
{
    uint32_t syscall = regs->eax;
    switch (syscall)
    {
        case EXIT:
            int code = regs->ebx;
            //TODO
            for(;;);
            break;
        
        case READ:
            uint32_t readResult = File_Read(regs->ebx, (uint8_t*)regs->ecx, regs->edx);
            set_eax(readResult);
            break;
        
        case WRITE:
            uint32_t writeResult = (uint32_t)VFS_Write(regs->ebx, (uint8_t*)regs->ecx, regs->edx);
            set_eax(writeResult);
            break;
        
        case OPEN:
            const char* path = (const char*)regs->ebx;
            uint32_t handle = File_Open(path, regs->ecx, regs->edx);
            set_eax(handle);
            break;
        
        case CLOSE:
            uint32_t result = File_Close(regs->ebx);
            set_eax(result);
            break;

        case LSEEK:
            int64_t offset = ((int64_t)(uint32_t)regs->edx << 32) | (uint32_t)regs->ecx;
            int64_t lseek = File_Seek(regs->ebx, offset, regs->esi);

            uint32_t low = (uint32_t)(lseek & 0xFFFFFFFF);
            uint32_t high = (uint32_t)((lseek >> 32) & 0xFFFFFFFF);

            set_eax(low);
            set_edx(high);
            break;
        
        case 0:
        default:
            log_warn("SYSCALL", "Unknown system call: 0x%x", syscall);
            set_eax(0);
            break;
    }
}