#pragma once

#include <core/Defs.h>
#include <stdint.h>

#define UNUSED_PORT         0x80

void ASMCALL outb(uint16_t port, uint8_t value);
uint8_t ASMCALL inb(uint16_t port);
uint16_t ASMCALL inw(uint16_t port);

void iowait()
{
    outb(UNUSED_PORT, 0);
}