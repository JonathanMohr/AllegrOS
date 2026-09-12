#include "isr.h"

#include <stddef.h>
#include "idt.h"

#include "../../panic/panic.h"

static volatile ISR_Handler isrHandlers[256] = {0};

void x86_ISR_Initialize(void)
{
    x86_ISR_InitializeGates();
    for (uint16_t i = 0; i < 256; i++)
        x86_IDT_EnableGate((uint8_t)i);
}

void x86_ISR_RegisterHandler(uint8_t interrupt, ISR_Handler handler)
{
    isrHandlers[interrupt] = handler;
}

void CDECL x86_ISR_Handler(const Registers* r)
{
    Registers regs = *r;
    if (regs.marker == 0xFFFFFFFF)
    {
        const uint32_t eflags = regs.ss;
        const uint32_t cs = regs.esp;
        const uint32_t eip = regs.eflags;
        const uint32_t error = regs.cs;
        const uint32_t interrupt = regs.eip;
        const uint32_t esp = regs.error;
        const uint32_t ss = regs.interrupt;
        regs.ss = ss;
        regs.esp = esp;
        regs.eflags = eflags;
        regs.cs = cs;
        regs.eip = eip;
        regs.error = error;
        regs.interrupt = interrupt;
    }

    if (regs.interrupt > 255)
    {
        PanicMessage("[KERNEL] Invalid interrupt 0x%uxd\n", regs.interrupt);
        Panic();
    }

    if (isrHandlers[regs.interrupt] != NULL)
        isrHandlers[regs.interrupt](&regs);

    else if (regs.interrupt >= 32)
    {
        PanicMessage("[KERNEL] Unhandled interrupt 0x%uxd\n", regs.interrupt);
    }

    else
    {
        PanicMessage("[KERNEL] Unhandled exception 0x%uxd\n", regs.interrupt);
        Panic();
    }
}

void x86_ISR_SetUser(uint8_t interrupt, bool allow)
{
    if (allow)
        x86_IDT_AddFlags(interrupt, IDT_FLAGS_RING3);
    else
        x86_IDT_RemoveFlags(interrupt, IDT_FLAGS_RING3);
}
