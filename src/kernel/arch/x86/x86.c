#include "x86.h"

#include "gdt.h"
#include "idt.h"
#include "isr.h"

void x86_Initialize()
{
    x86_GDT_Initialize();
    x86_IDT_Initialize();
    x86_ISR_Initialize();
}
