#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <abi.h>

typedef struct Registers {
    uint32_t ds, es, fs, gs;

    uint32_t edi, esi, ebp, useless_esp, ebx, edx, ecx, eax;

    uint32_t marker;
    
    uint32_t interrupt, error;

    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed)) Registers;

typedef void (*ISR_Handler)(const Registers* regs);

void x86_ISR_InitializeGates(void);

void x86_ISR_Initialize(void);
void x86_ISR_RegisterHandler(uint8_t interrupt, ISR_Handler handler);
void x86_ISR_SetUser(uint8_t interrupt, bool allow);
