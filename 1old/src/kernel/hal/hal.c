#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/isr.h"
#include "arch/i686/irq.h"
#include "arch/i686/user.h"

#include "stack/stack.h"
#include "syscalls/syscall.h"

void HAL_Initialize()
{
    i686_GDT_Initialize((uintptr_t)kernelStackTop);
    i686_IDT_Initialize();
    i686_ISR_Initialize();
    i686_IRQ_Initialize();

    i686_ISR_RegisterHandler(0x80, syscall);
}