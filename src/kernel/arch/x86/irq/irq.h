#pragma once

#include "../isr.h"
#include <stdbool.h>

typedef void (*IRQ_Handler)(const Registers* regs);

void x86_IRQ_Initialize();
bool x86_IRQ_RegisterHandler(uint8_t irq, ISR_Handler handler);
bool x86_PIT_Timer_Initialize(uint32_t frequency, ISR_Handler handler);
