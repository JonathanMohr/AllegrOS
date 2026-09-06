#pragma once

#include "../isr.h"

typedef void (*IRQ_Handler)(const Registers* regs);

void x86_IRQ_Initialize();
void x86_IRQ_RegisterHandler(uint8_t irq, ISR_Handler handler);
