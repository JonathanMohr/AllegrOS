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

void* CDECL memcpy(void* dst, const void* src, uint32_t count);
void* CDECL memset(void* dst, int32_t value, uint32_t count);
int32_t CDECL memcmp(const char* a, const char* b, uint64_t n);

uint32_t CDECL strlen(const char* str);
char* CDECL strchr(const char* str, int32_t c);

int8_t CDECL toupper(int8_t c);
