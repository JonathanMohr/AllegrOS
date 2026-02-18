#pragma once

#include <stdint.h>
#include <abi.h>

void CDECL x86_outb(uint16_t port, uint8_t value);
uint8_t CDECL x86_inb(uint16_t port);


typedef struct 
{
    uint64_t Base;
    uint64_t Length;
    uint32_t Type;
    uint32_t ACPI;

} x86_E820MemoryBlock;

#define x86_E820_USABLE           1
#define x86_E820_RESERVED         2
#define x86_E820_ACPI_RECLAIMABLE 3
#define x86_E820_ACPI_NVS         4
#define x86_E820_BAD_MEMORY       5

int CDECL x86_E820GetNextBlock(x86_E820MemoryBlock* block, uint32_t* continuationId);
