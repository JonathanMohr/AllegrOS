#pragma once

#include <stdint.h>
#include <abi.h>

typedef struct GDTEntry {
    uint16_t limitLow;      // limit (bits 0-15)
    uint16_t baseLow;       // base (bits 0-15)
    uint8_t baseMiddle;     // base (bits 16-23)
    uint8_t access;         // access
    uint8_t flagsLimitHigh; // limit (bits 16-19) | flags
    uint8_t baseHigh;       // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct GDTDescriptor {
    uint16_t limit; // sizeof(gdt) - 1
    uint32_t ptr;   // address of GDT
} __attribute__((packed)) GDTDescriptor;


#define GDT_ACCESS_PRESENT 0x80

#define GDT_ACCESS_RING0 0x00
#define GDT_ACCESS_RING1 0x20
#define GDT_ACCESS_RING2 0x40
#define GDT_ACCESS_RING3 0x60

#define GDT_ACCESS_DESCRIPTOR_TYPE_NORMAL 0x10
#define GDT_ACCESS_DESCRIPTOR_TYPE_SYSTEM 0x00

#define GDT_ACCESS_EXECUTABLE 0x08

#define GDT_ACCESS_DIRECTION_CODE_CONFORMING     0x04
#define GDT_ACCESS_DIRECTION_CODE_NON_CONFORMING 0x00
#define GDT_ACCESS_DIRECTION_DATA_EXPAND_DOWN    0x04
#define GDT_ACCESS_DIRECTION_DATA_NORMAL         0x00

#define GDT_ACCESS_CODE_READABLE  0x02
#define GDT_ACCESS_DATA_WRITEABLE 0x02

#define GDT_ACCESS_ACCESSED 0x01


#define GDT_FLAGS_GRANULARITY_1B 0x00
#define GDT_FLAGS_GRANULARITY_4K 0x80

#define GDT_FLAG_SIZE_32   0x40
#define GDT_FLAG_LONG_MODE 0x20

#define GDT_FLAGS_AVAILABLE 0x10


#define GDT_GET_SEGMENT_OFFSET(index) ((index) * 8)

#define GDT_CODE_SEGMENT (GDT_GET_SEGMENT_OFFSET(1))
#define GDT_DATA_SEGMENT (GDT_GET_SEGMENT_OFFSET(2))

void CDECL x86_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void x86_GDT_Initialize();
