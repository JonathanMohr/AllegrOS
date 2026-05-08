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

} __attribute__((packed)) x86_E820MemoryBlock;

uint32_t CDECL strlen(const char* str);
char* CDECL strchr(const char* str, int32_t c);

int8_t CDECL toupper(int8_t c);

void CDECL x86_invlpg(void* addr);
