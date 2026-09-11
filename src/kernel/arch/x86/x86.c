#include "x86.h"

#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq/irq.h"

void x86_Initialize(void)
{
    x86_GDT_Initialize();
    x86_IDT_Initialize();
    x86_ISR_Initialize();
    x86_IRQ_Initialize();
    __asm__ volatile("sti");
}
