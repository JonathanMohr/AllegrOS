#include "ata.h"

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

#define ATA_DRIVE_SELECT_BASE       0xA0
#define ATA_DRIVE_SELECT_LBA_BASE   0xE0
#define ATA_DRIVE_SELECT_SLAVE_BIT  0x10

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

#define ATA_COMMAND_IDENTIFY        0xEC
#define ATA_COMMAND_READ_PIO        0x20
#define ATA_COMMAND_WRITE_PIO       0x30
#define ATA_COMMAND_CACHE_FLUSH     0xE7
#define ATA_COMMAND_READ_PIO_EXT    0x24
#define ATA_COMMAND_WRITE_PIO_EXT   0x34

#define ATA_IDENTIFY_SUCCESS        0
#define ATA_IDENTIFY_DOES_NOT_EXIST 1
#define ATA_IDENTIFY_NOT_STANDARD   2
#define ATA_IDENTIFY_ERROR          3

static int ata_identify(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint16_t* buffer /* 256 uint16_t */)
{
    x86_outb(ioBase + ATA_REG_HDDEVSEL, ATA_DRIVE_SELECT_BASE | (slave << 4));
    ata_ioWait(ctrlBase);

    x86_outb(ioBase + ATA_REG_SECCOUNT0, 0);
    x86_outb(ioBase + ATA_REG_LBA0, 0);
    x86_outb(ioBase + ATA_REG_LBA1, 0);
    x86_outb(ioBase + ATA_REG_LBA2, 0);

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_IDENTIFY);

    uint8_t status = x86_inb(ioBase + ATA_REG_STATUS);
    if (status == 0)
    {
        return ATA_IDENTIFY_DOES_NOT_EXIST;
    }

    ata_waitBusy(ioBase);

    uint8_t lba1 = x86_inb(ioBase + ATA_REG_LBA1);
    uint8_t lba2 = x86_inb(ioBase + ATA_REG_LBA2);
    if (lba1 != 0 || lba2 != 0) return ATA_IDENTIFY_NOT_STANDARD;

    if (ata_waitDatarequest(ioBase) != 0) return ATA_IDENTIFY_ERROR;

    for (uint16_t i = 0; i < 256; i++)
        buffer[i] = x86_inw(ioBase + ATA_REG_DATA);

    return ATA_IDENTIFY_SUCCESS;
}

static uint16_t ata_read28(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint32_t lba, uint8_t sectorCount, uint32_t wordsPerSector, uint16_t* buffer)
{
    ata_waitBusy(ioBase);

    x86_outb(ioBase + ATA_REG_HDDEVSEL, ATA_DRIVE_SELECT_LBA_BASE | (slave << 4) | ((lba >> 24) & 0x0F));
    ata_ioWait(ctrlBase);

    x86_outb(ioBase + ATA_REG_SECCOUNT0, sectorCount);
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_READ_PIO);

    uint16_t sectorsToRead = (sectorCount == 0) ? 256 : sectorCount;

    for (uint16_t s = 0; s < sectorsToRead; s++)
    {
        if (ata_waitDatarequest(ioBase) != 0)
            return s;

        for (uint32_t i = 0; i < wordsPerSector; i++)
            buffer[s * wordsPerSector + i] = x86_inw(ioBase + ATA_REG_DATA);
    }

    return sectorsToRead;
}

static uint32_t ata_read48(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint64_t lba, uint16_t sectorCount, uint32_t wordsPerSector, uint16_t* buffer)
{
    ata_waitBusy(ioBase);

    x86_outb(ioBase + ATA_REG_HDDEVSEL, ATA_DRIVE_SELECT_LBA_BASE | (slave << 4));
    ata_ioWait(ctrlBase);

    x86_outb(ioBase + ATA_REG_SECCOUNT0, sectorCount >> 8);
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba >> 24));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 32));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 40));

    x86_outb(ioBase + ATA_REG_SECCOUNT0, (uint8_t)(sectorCount));
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_READ_PIO_EXT);

    uint32_t sectorsToRead = (sectorCount == 0) ? 256 : sectorCount;

    for (uint32_t s = 0; s < sectorsToRead; s++)
    {
        if (ata_waitDatarequest(ioBase) != 0)
            return s;

        for (uint32_t i = 0; i < wordsPerSector; i++)
            buffer[s * wordsPerSector + i] = x86_inw(ioBase + ATA_REG_DATA);
    }

    return sectorsToRead;
}

static uint16_t ata_write28(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint32_t lba, uint8_t sectorCount, uint32_t wordsPerSector, const uint16_t* buffer)
{
    ata_waitBusy(ioBase);

    x86_outb(ioBase + ATA_REG_HDDEVSEL, ATA_DRIVE_SELECT_LBA_BASE | (slave << 4) | ((lba >> 24) & 0x0F));
    ata_ioWait(ctrlBase);

    x86_outb(ioBase + ATA_REG_SECCOUNT0, sectorCount);
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_WRITE_PIO);

    uint16_t sectorsToWrite = (sectorCount == 0) ? 256 : sectorCount;

    for (uint16_t s = 0; s < sectorsToWrite; s++)
    {
        if (ata_waitDatarequest(ioBase) != 0)
            return s;

        for (uint32_t i = 0; i < wordsPerSector; i++)
            x86_outw(ioBase + ATA_REG_DATA, buffer[s * wordsPerSector + i]);
    }

    return sectorsToWrite;
}

static uint32_t ata_write48(uint16_t ioBase, uint16_t ctrlBase, uint8_t slave, uint64_t lba, uint16_t sectorCount, uint32_t wordsPerSector, const uint16_t* buffer)
{
    ata_waitBusy(ioBase);

    x86_outb(ioBase + ATA_REG_HDDEVSEL, ATA_DRIVE_SELECT_LBA_BASE | (slave << 4));
    ata_ioWait(ctrlBase);

    x86_outb(ioBase + ATA_REG_SECCOUNT0, sectorCount >> 8);
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba >> 24));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 32));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 40));

    x86_outb(ioBase + ATA_REG_SECCOUNT0, (uint8_t)(sectorCount));
    x86_outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba));
    x86_outb(ioBase + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    x86_outb(ioBase + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_WRITE_PIO_EXT);

    uint32_t sectorsToWrite = (sectorCount == 0) ? 256 : sectorCount;

    for (uint32_t s = 0; s < sectorsToWrite; s++)
    {
        if (ata_waitDatarequest(ioBase) != 0)
            return s;

        for (uint32_t i = 0; i < wordsPerSector; i++)
            x86_outw(ioBase + ATA_REG_DATA, buffer[s * wordsPerSector + i]);
    }

    x86_outb(ioBase + ATA_REG_COMMAND, ATA_COMMAND_CACHE_FLUSH);
    ata_waitBusy(ioBase);

    return sectorsToWrite;
}
