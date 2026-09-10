#pragma once

#include "../isr.h"
#include <stdbool.h>

typedef bool (*IRQ_Handler)(const Registers* regs);

void x86_IRQ_Initialize(void);
void x86_IRQ_Send_EOI(uint8_t irq);
bool x86_IRQ_RegisterHandler(uint8_t irq, IRQ_Handler handler);
bool x86_PIT_Timer_Initialize(uint32_t frequency, IRQ_Handler handler);
