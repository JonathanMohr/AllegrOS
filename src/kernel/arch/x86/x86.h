#pragma once

#include <stdint.h>
#include <abi.h>

// TODO: Check
#define UNUSED_PORT 0x80

void x86_Initialize(void);

void CDECL x86_outb(uint16_t port, uint8_t value);
uint8_t CDECL x86_inb(uint16_t port);
void CDECL x86_outw(uint16_t port, uint16_t value);
uint16_t CDECL x86_inw(uint16_t port);
void CDECL x86_outl(uint16_t port, uint32_t value);
uint32_t CDECL x86_inl(uint16_t port);
static inline void x86_iowait(void) { x86_outb(UNUSED_PORT, 0); }
