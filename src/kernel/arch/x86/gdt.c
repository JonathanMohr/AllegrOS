#include "gdt.h"

typedef struct
{
    uint32_t prevTaskLink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp;
    uint32_t esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomapBase;
} __attribute__((packed)) TSS;

#define GDT_LIMIT_LOW(limit)               ((limit) & 0xFFFF)
#define GDT_BASE_LOW(base)                 ((base) & 0xFFFF)
#define GDT_BASE_MIDDLE(base)              (((base) >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags) ((((limit) >> 16) & 0xF) | ((flags) & 0xF0))
#define GDT_BASE_HIGH(base)                (((base) >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit),                       \
    GDT_BASE_LOW(base),                         \
    GDT_BASE_MIDDLE(base),                      \
    (access),                                   \
    GDT_FLAGS_LIMIT_HIGH(limit, flags),         \
    GDT_BASE_HIGH(base)                         \
}

static TSS tss;

static GDTEntry gdt[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    GDT_ENTRY(0,
              0xFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR_TYPE_NORMAL | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_SIZE_32 | GDT_FLAGS_GRANULARITY_4K),

    // Kernel 32-bit data segment
    GDT_ENTRY(0,
              0xFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR_TYPE_NORMAL | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_SIZE_32 | GDT_FLAGS_GRANULARITY_4K),

    // User 32-bit code segment
    GDT_ENTRY(0,
              0xFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR_TYPE_NORMAL | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_SIZE_32 | GDT_FLAGS_GRANULARITY_4K),

    // User 32-bit data segment
    GDT_ENTRY(0,
              0xFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR_TYPE_NORMAL | GDT_ACCESS_DATA_WRITEABLE,
              GDT_FLAG_SIZE_32 | GDT_FLAGS_GRANULARITY_4K),

    // TSS descriptor
    GDT_ENTRY(0, 0, 0, 0),
};

GDTDescriptor gdtDescriptor = {
    .limit = sizeof(gdt) - 1,
    .ptr = (uint32_t)&gdt
};

void x86_GDT_Initialize(void)
{
    uint32_t tssBase = (uint32_t)&tss;
    uint32_t tssLimit = sizeof(TSS) - 1;

    gdt[5].limitLow = GDT_LIMIT_LOW(tssLimit);
    gdt[5].baseLow = GDT_BASE_LOW(tssBase);
    gdt[5].baseMiddle = GDT_BASE_MIDDLE(tssBase);
    gdt[5].access = GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | 0x09;
    gdt[5].flagsLimitHigh = GDT_FLAGS_LIMIT_HIGH(tssLimit, 0);
    gdt[5].baseHigh = GDT_BASE_HIGH(tssBase);

    x86_GDT_Load(&gdtDescriptor, GDT_GET_SEGMENT_OFFSET(1), GDT_GET_SEGMENT_OFFSET(2));

    tss.ss0 = GDT_GET_SEGMENT_OFFSET(2);
    tss.esp0 = 0;

    x86_TSS_Load(GDT_GET_SEGMENT_OFFSET(5));
}

void x86_GDT_ChangeStack(void* stackPointer)
{
    tss.esp0 = (uint32_t)stackPointer;
}
