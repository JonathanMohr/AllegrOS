#pragma once

#include <stdint.h>
#include <abi.h>

typedef struct IDTEntry {
    uint16_t baseLow;        // base (bits 0-15)
    uint16_t segmentSelector; // segment selector
    uint8_t reserved;        // 0
    uint8_t flags;           // flags
    uint16_t baseHigh;       // base (bits 16-31)
} __attribute__((packed)) IDTEntry;

typedef struct IDTDescriptor {
    uint16_t limit; // sizeof(idt) - 1
    uint32_t ptr;   // address of IDT
} __attribute__((packed)) IDTDescriptor;


#define IDT_FLAGS_PRESENT 0x80

#define IDT_FLAGS_RING0 0x00
#define IDT_FLAGS_RING1 0x20
#define IDT_FLAGS_RING2 0x40
#define IDT_FLAGS_RING3 0x60

#define IDT_FLAGS_GATE_TASK       0x5
#define IDT_FLAGS_GATE_16BIT_INT  0x6
#define IDT_FLAGS_GATE_16BIT_TRAP 0x7
#define IDT_FLAGS_GATE_32BIT_INT  0xE
#define IDT_FLAGS_GATE_32BIT_TRAP 0xF


void CDECL x86_IDT_Load(IDTDescriptor* descriptor);

void x86_IDT_Initialize(void);
void x86_IDT_EnableGate(uint8_t interrupt);
void x86_IDT_DisableGate(uint8_t interrupt);
void x86_IDT_AddFlags(uint8_t interrupt, uint8_t flags);
void x86_IDT_RemoveFlags(uint8_t interrupt, uint8_t flags);
void x86_IDT_SetGate(uint8_t interrupt, void* base, uint16_t selector, uint8_t flags);
