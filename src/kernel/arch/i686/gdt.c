#include "gdt.h"

#include <core/Defs.h>

typedef struct __attribute__((packed)) {
    uint16_t prev_task_link;
    uint16_t reserved0;

    uint32_t esp0;       // Stack Pointer Ring 0
    uint16_t ss0;        // Stack Segment Ring 0
    uint16_t reserved1;

    uint32_t esp1;       // Stack Pointer Ring 1 (optional)
    uint16_t ss1;        // Stack Segment Ring 1
    uint16_t reserved2;

    uint32_t esp2;       // Stack Pointer Ring 2 (optional)
    uint16_t ss2;        // Stack Segment Ring 2
    uint16_t reserved3;

    uint32_t cr3;        // Page Directory Base Register (CR3)

    uint32_t eip;        // Instruction Pointer (bei Task Switch, selten benutzt)
    uint32_t eflags;     // Flags Register

    uint32_t eax;        // Allgemeine Register
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp;        // Stack Pointer (allgemein)
    uint32_t ebp;        // Base Pointer
    uint32_t esi;
    uint32_t edi;

    uint16_t es;         // Segment Register
    uint16_t reserved4;

    uint16_t cs;
    uint16_t reserved5;

    uint16_t ss;
    uint16_t reserved6;

    uint16_t ds;
    uint16_t reserved7;

    uint16_t fs;
    uint16_t reserved8;

    uint16_t gs;
    uint16_t reserved9;

    uint16_t ldt_selector;   // Selector für LDT (optional)
    uint16_t reserved10;

    uint16_t debug_trap : 1;
    uint16_t reserved11 : 15;

    uint16_t io_map_base;    // Offset zur I/O-Map (oft sizeof(TSS))
} TSS;

typedef struct
{
    uint16_t LimitLow;                  // limit (bits 0-15)
    uint16_t BaseLow;                   // base (bits 0-15)
    uint8_t BaseMiddle;                 // base (bits 16-23)
    uint8_t Access;                     // access
    uint8_t FlagsLimitHi;               // limit (bits 16-19) | flags
    uint8_t BaseHigh;                   // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct
{
    uint16_t Limit;                     // sizeof(gdt) - 1
    GDTEntry* Ptr;                      // address of GDT
} __attribute__((packed)) GDTDescriptor;

typedef enum
{
    GDT_ACCESS_CODE_READABLE                = 0x02,
    GDT_ACCESS_DATA_WRITEABLE               = 0x02,

    GDT_ACCESS_CODE_CONFORMING              = 0x04,
    GDT_ACCESS_DATA_DIRECTION_NORMAL        = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN          = 0x04,

    GDT_ACCESS_DATA_SEGMENT                 = 0x10,
    GDT_ACCESS_CODE_SEGMENT                 = 0x18,

    GDT_ACCESS_DESCRIPTOR_TSS               = 0x00,

    GDT_ACCESS_RING0                        = 0x00,
    GDT_ACCESS_RING1                        = 0x20,
    GDT_ACCESS_RING2                        = 0x40,
    GDT_ACCESS_RING3                        = 0x60,

    GDT_ACCESS_PRESENT                      = 0x80,

} GDT_ACCESS;

typedef enum 
{
    GDT_FLAG_64BIT                          = 0x20,
    GDT_FLAG_32BIT                          = 0x40,
    GDT_FLAG_16BIT                          = 0x00,

    GDT_FLAG_GRANULARITY_1B                 = 0x00,
    GDT_FLAG_GRANULARITY_4K                 = 0x80,
} GDT_FLAGS;

// Helper macros
#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)    (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) {                     \
    GDT_LIMIT_LOW(limit),                                           \
    GDT_BASE_LOW(base),                                             \
    GDT_BASE_MIDDLE(base),                                          \
    access,                                                         \
    GDT_FLAGS_LIMIT_HI(limit, flags),                               \
    GDT_BASE_HIGH(base)                                             \
}

GDTEntry g_GDT[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment (Ring 0)
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // Kernel 32-bit data segment (Ring 0)
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit code segment (Ring 3)
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // User 32-bit data segment (Ring 3)
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    // TSS Descriptor (placeholder, will be set at runtime)
    GDT_ENTRY(0, 0,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | 0x09,   // 0x09 = 32-bit available TSS
              0x00),

};

GDTDescriptor g_GDTDescriptor = { sizeof(g_GDT) - 1, g_GDT};
TSS g_TSS = {0};

void ASMCALL i686_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);
void ASMCALL i686_TSS_Load(uint16_t selector);

void tss_initialize(uintptr_t kernelStackTop)
{
    g_TSS.esp0 = kernelStackTop;
    g_TSS.ss0 = i686_GDT_DATA_SEGMENT; // Kernel-Datensegment-Selector
    g_TSS.io_map_base = sizeof(TSS);   // Kein IO-Map
}

void i686_GDT_Initialize(uintptr_t kernelStackTop)
{
    tss_initialize(kernelStackTop);

    // Set up the TSS descriptor at runtime
    uintptr_t tss_base = (uintptr_t)&g_TSS;
    uint32_t tss_limit = sizeof(TSS) - 1;
    g_GDT[5].LimitLow = GDT_LIMIT_LOW(tss_limit);
    g_GDT[5].BaseLow = GDT_BASE_LOW(tss_base);
    g_GDT[5].BaseMiddle = GDT_BASE_MIDDLE(tss_base);
    g_GDT[5].Access = GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | 0x09; // 0x09 = 32-bit available TSS
    g_GDT[5].FlagsLimitHi = GDT_FLAGS_LIMIT_HI(tss_limit, 0x00);
    g_GDT[5].BaseHigh = GDT_BASE_HIGH(tss_base);

    i686_GDT_Load(&g_GDTDescriptor, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);

    i686_TSS_Load(5 * 8);
}