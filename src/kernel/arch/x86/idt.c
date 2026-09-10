#include "idt.h"

IDTEntry idt[256] = {0};

IDTDescriptor idtDescriptor = {
    .limit = sizeof(idt) - 1,
    .ptr = (uint32_t)&idt
};

void x86_IDT_Initialize(void)
{
    x86_IDT_Load(&idtDescriptor);
}

void x86_IDT_EnableGate(uint8_t interrupt)
{
    idt[interrupt].flags |= IDT_FLAGS_PRESENT;
}

void x86_IDT_DisableGate(uint8_t interrupt)
{
    idt[interrupt].flags &= ~IDT_FLAGS_PRESENT;
}

void x86_IDT_AddFlags(uint8_t interrupt, uint8_t flags)
{
    idt[interrupt].flags |= flags;
}

void x86_IDT_RemoveFlags(uint8_t interrupt, uint8_t flags)
{
    idt[interrupt].flags &= ~flags;
}

void x86_IDT_SetGate(uint8_t interrupt, void* base, uint16_t selector, uint8_t flags)
{
    idt[interrupt].baseLow = ((uint32_t)base) & 0xFFFF;
    idt[interrupt].segmentSelector = selector;
    idt[interrupt].reserved = 0;
    idt[interrupt].flags = flags;
    idt[interrupt].baseHigh = ((uint32_t)base >> 16) & 0xFFFF;
}
