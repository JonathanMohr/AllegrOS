#pragma once

#include <stdint.h>
#include <abi.h>

void CDECL x86_outb(uint16_t port, uint8_t value);
uint8_t CDECL x86_inb(uint16_t port);
