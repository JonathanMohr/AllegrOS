#include "gdt.h"

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

#define GDT_GET_SEGMENT_OFFSET(index) ((index) * 8)

GDTEntry gdt[] = {
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
};

GDTDescriptor gdtDescriptor = {
    .limit = sizeof(gdt) - 1,
    .ptr = (uint32_t)&gdt
};

void x86_GDT_Initialize()
{
    x86_GDT_Load(&gdtDescriptor, GDT_GET_SEGMENT_OFFSET(1), GDT_GET_SEGMENT_OFFSET(2));
}
