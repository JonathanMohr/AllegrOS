#include "isr.h"

#include <stddef.h>
#include "idt.h"

#include "../../panic/panic.h"

ISR_Handler isrHandlers[256] = {0};

void x86_ISR_Initialize()
{
    x86_ISR_InitializeGates();
    for (uint16_t i = 0; i < 256; i++)
        x86_IDT_EnableGate((uint8_t)i);
}

void x86_ISR_RegisterHandler(uint8_t interrupt, ISR_Handler handler)
{
    isrHandlers[interrupt] = handler;
}

void CDECL x86_ISR_Handler(const Registers* regs)
{
    if (regs->interrupt > 255)
    {
        PanicMessage("[KERNEL] Invalid interrupt 0x%uxd\n", regs->interrupt);
        Panic();
    }

    if (isrHandlers[regs->interrupt] != NULL)
        isrHandlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
    {
        PanicMessage("[KERNEL] Unhandled interrupt 0x%uxd\n", regs->interrupt);
    }

    else
    {
        PanicMessage("[KERNEL] Unhandled exception 0x%uxd\n", regs->interrupt);
        Panic();
    }
}
