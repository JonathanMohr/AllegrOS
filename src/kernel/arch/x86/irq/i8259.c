#include "i8259.h"

#include "../x86.h"

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT    0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT    0xA1

#define PIC_ICW1_ICW4       0x01
#define PIC_ICW1_SINGLE     0x02
#define PIC_ICW1_INTERVAL4  0x04
#define PIC_ICW1_LEVEL      0x08
#define PIC_ICW1_INITIALIZE 0x10

#define PIC_ICW4_8086          0x01
#define PIC_ICW4_AUTO_EOI      0x02
#define PIC_ICW4_BUFFER_MASTER 0x04
#define PIC_ICW4_BUFFERED      0x08
#define PIC_ICW4_SFNM          0x10

#define PIC_CMD_END_OF_INTERRUPT 0x20
#define PIC_CMD_READ_IRR         0x0A
#define PIC_CMD_READ_ISR         0x0B

static uint16_t picMask = 0xFFFF;
static bool autoEoi = false;

void x86_i8259_SetMask(uint16_t newMask)
{
    picMask = newMask;
    x86_outb(PIC1_DATA_PORT, (uint8_t)(picMask & 0xFF));
    x86_iowait();
    x86_outb(PIC2_DATA_PORT, (uint8_t)(picMask >> 8));
    x86_iowait();
}

uint16_t x86_i8259_GetMask()
{
    return ((uint16_t)x86_inb(PIC1_DATA_PORT)) | ((uint16_t)(x86_inb(PIC2_DATA_PORT)) << 8);
}


void x86_i8259_Disable()
{
    x86_i8259_SetMask(0xFFFF);
}


bool x86_i8259_Probe()
{
    x86_i8259_Disable();
    x86_i8259_SetMask(0x1337);
    return x86_i8259_GetMask() == 0x1337;
}

void x86_i8259_Configure(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEoi)
{
    // Mask everything
    x86_i8259_SetMask(0xFFFF);

    // initialization control word 1
    x86_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    x86_iowait();
    x86_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    x86_iowait();

    // initialization control word 2 - the offsets
    x86_outb(PIC1_DATA_PORT, offsetPic1);
    x86_iowait();
    x86_outb(PIC2_DATA_PORT, offsetPic2);
    x86_iowait();

    // initialization control word 3
    x86_outb(PIC1_DATA_PORT, 0x4);             // tell PIC1 that it has a slave at IRQ2 (0000 0100)
    x86_iowait();
    x86_outb(PIC2_DATA_PORT, 0x2);             // tell PIC2 its cascade identity (0000 0010)
    x86_iowait();

    // initialization control word 4
    uint8_t icw4 = PIC_ICW4_8086;
    if (autoEoi)
        icw4 |= PIC_ICW4_AUTO_EOI;

    x86_outb(PIC1_DATA_PORT, icw4);
    x86_iowait();
    x86_outb(PIC2_DATA_PORT, icw4);
    x86_iowait();

    // mask all interrupts until they are enabled by the device driver
    x86_i8259_SetMask(0xFFFF);
}

void x86_i8259_SendEndOfInterrupt(uint8_t irq)
{
    if (irq >= 8)
        x86_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    x86_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void x86_i8259_Mask(uint8_t irq)
{
    x86_i8259_SetMask(picMask | (1 << irq));
}

void x86_i8259_Unmask(uint8_t irq)
{
    x86_i8259_SetMask(picMask & ~(1 << irq));
}

static const PIC_Driver picDriver = {
    .name = "8259 PIC",
    .Probe = &x86_i8259_Probe,
    .Initialize = &x86_i8259_Configure,
    .Disable = &x86_i8259_Disable,
    .SendEndOfInterrupt = &x86_i8259_SendEndOfInterrupt,
    .Mask = &x86_i8259_Mask,
    .Unmask = &x86_i8259_Unmask
};

const PIC_Driver* x86_i8259_GetDriver()
{
    return &picDriver;
}
