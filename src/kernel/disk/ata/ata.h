#pragma once

#include "../disk.h"
#include "../../arch/x86/x86.h"
#include <stdint.h>

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT0   0x02
#define ATA_REG_LBA0        0x03
#define ATA_REG_LBA1        0x04
#define ATA_REG_LBA2        0x05
#define ATA_REG_HDDEVSEL    0x06
#define ATA_REG_COMMAND     0x07
#define ATA_REG_STATUS      0x07

#define ATA_REG_CONTROL     0x00
#define ATA_REG_ALTSTATUS   0x00

#define ATA_SR_BUSY         0x80
#define ATA_SR_DRIVEREADY   0x40
#define ATA_SR_DRIVEFAULT   0x20
#define ATA_SR_DATAREQUEST  0x08
#define ATA_SR_ERROR        0x01

static void ata_waitBusy(uint16_t ioBase)
{
    while (x86_inb(ioBase + ATA_REG_STATUS) & ATA_SR_BUSY);
}

static int ata_waitDatarequest(uint16_t ioBase)
{
    while (1)
    {
        uint8_t status = x86_inb(ioBase + ATA_REG_STATUS);
        if (status & ATA_SR_ERROR) return 1;
        if (status & ATA_SR_DATAREQUEST) return 0;
    }
}

static void ata_ioWait(uint16_t ctrlBase)
{
    x86_inb(ctrlBase + ATA_REG_ALTSTATUS);
    x86_inb(ctrlBase + ATA_REG_ALTSTATUS);
    x86_inb(ctrlBase + ATA_REG_ALTSTATUS);
    x86_inb(ctrlBase + ATA_REG_ALTSTATUS);
}

#define ATA_COMMAND_IDENTIFY 0xEC

int ata_identify(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint16_t* buffer /* 256 */)
{
    x86_outb(ioBase + ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));
    ata_ioWait(ctrlBase);

    // TODO: ...
}
