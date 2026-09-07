#include "irq.h"
#include "pic.h"

#include "i8259.h"
#include "../x86.h"

#include "../../../panic/panic.h"

#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20

static IRQ_Handler irqHandlers[16] = {0};
static const PIC_Driver* picDriver = NULL;

void x86_IRQ_Handler(const Registers* regs)
{
    const uint32_t irq = regs->interrupt - PIC_REMAP_OFFSET;

    if (irq > 15)
    {
        PanicMessage("[KERNEL] Invalid PIC interrupt %udd\n", irq);
        Panic();
    }

    if (irqHandlers[irq] != NULL)
    {
        // handle IRQ
        irqHandlers[irq](regs);
    }
    else
    {
        PanicMessage("[KERNEL] Unhandled IRQ %udd\n", irq);
    }

    picDriver->SendEndOfInterrupt(irq);
}

void x86_IRQ_Initialize()
{
    const PIC_Driver* drivers[] = {
        x86_i8259_GetDriver()
    };

    for (uint64_t i = 0; i < (sizeof(drivers) / sizeof(drivers[0])); i++)
    {
        if (drivers[i]->Probe())
        {
            picDriver = drivers[i];
            break;
        }
    }

    if (picDriver == NULL)
    {
        PanicMessage("[KERNEL] No PIC found!\n");
        Panic();
    }

    picDriver->Initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

    // register ISR handlers for each of the 16 irq lines
    for (uint8_t i = 0; i < 16; i++)
        x86_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, x86_IRQ_Handler);
}

bool x86_IRQ_RegisterHandler(uint8_t irq, ISR_Handler handler)
{
    if (irq > 15)
    {
        // PanicMessage("[KERNEL] IRQ too high to register handler\n");
        // Panic();
        return false;
    }

    irqHandlers[irq] = handler;
    picDriver->Unmask(irq);

    return true;
}

bool x86_PIT_Timer_Initialize(uint32_t frequency, ISR_Handler handler)
{
    uint32_t divisor = 1193182 / frequency;

    x86_outb(0x43, 0x36);
    x86_outb(0x40, (uint8_t)(divisor & 0xFF));
    x86_outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    if (!x86_IRQ_RegisterHandler(0, handler))
        return false;

    return true;
}
