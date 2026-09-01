#include "ata.h"

#include "../../../arch/x86/x86.h"
#include "../../../memory/memory.h"
#include <stdint.h>
#include <memory.h>

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

static void ATA_ExtractModelName(const uint16_t* identifyBuf, char* out)
{
    for (uint8_t i = 0; i < 20; i++)
    {
        uint16_t word = identifyBuf[27 + i];
        out[i * 2]     = (char)(word >> 8);
        out[i * 2 + 1] = (char)(word & 0xFF);
    }
    out[40] = '\0';
    for (int8_t i = 39; i >= 0 && out[i] == ' '; i--)
        out[i] = '\0';
}


typedef struct ATA_Data
{
    uint16_t ioBase;
    uint16_t ctrlBase;
    uint8_t slave;
    bool supportsLBA48;
} ATA_Data;

bool ATA_CheckPCIDevice(const PCI_Device* device)
{
    return device->classCode == 0x01 && device->subclass == 0x01;
}

uint64_t ATA_Read(Block_Device* dev, uint64_t sector, uint64_t count, void* buffer)
{
    if (sector >= dev->sectorCount || count == 0)
        return 0;

    if (count > dev->sectorCount - sector)
        count = dev->sectorCount - sector;

    ATA_Data* ataData = (ATA_Data*)dev->data;
    uint16_t* buf16 = (uint16_t*)buffer;
    uint64_t totalRead = 0;
    const uint32_t wordsPerSector = (uint32_t)(dev->sectorSize / 2);
    const uint32_t maxChunk = ataData->supportsLBA48 ? 65536 : 256;

    while (totalRead < count)
    {
        const uint64_t remaining = count - totalRead;
        const uint32_t chunk = (remaining > maxChunk) ? maxChunk : (uint32_t)remaining;

        uint32_t result;
        if (ataData->supportsLBA48)
        {
            const uint16_t regCount = (chunk == 65536) ? 0 : (uint16_t)chunk;
            result = ata_read48(ataData->ioBase, ataData->ctrlBase, ataData->slave, sector + totalRead, regCount, wordsPerSector, buf16);
        }
        else
        {
            const uint8_t regCount = (chunk == 256) ? 0 : (uint8_t)chunk;
            result = (uint32_t)ata_read28(ataData->ioBase, ataData->ctrlBase, ataData->slave, (uint32_t)(sector + totalRead), regCount, wordsPerSector, buf16);
        }

        buf16 += result * wordsPerSector;
        totalRead += result;

        if ((uint32_t)result != chunk) break;
    }

    return totalRead;
}

uint64_t ATA_Write(Block_Device* dev, uint64_t sector, uint64_t count, const void* buffer)
{
    if (sector >= dev->sectorCount || count == 0)
        return 0;

    if (count > dev->sectorCount - sector)
        count = dev->sectorCount - sector;

    ATA_Data* ataData = (ATA_Data*)dev->data;
    const uint16_t* buf16 = (uint16_t*)buffer;
    uint64_t totalWritten = 0;
    const uint32_t wordsPerSector = (uint32_t)(dev->sectorSize / 2);
    const uint32_t maxChunk = ataData->supportsLBA48 ? 65536 : 256;

    while (totalWritten < count)
    {
        const uint64_t remaining = count - totalWritten;
        const uint32_t chunk = (remaining > maxChunk) ? maxChunk : (uint32_t)remaining;

        uint32_t result;
        if (ataData->supportsLBA48)
        {
            const uint16_t regCount = (chunk == 65536) ? 0 : (uint16_t)chunk;
            result = ata_write48(ataData->ioBase, ataData->ctrlBase, ataData->slave, sector + totalWritten, regCount, wordsPerSector, buf16);
        }
        else
        {
            const uint8_t regCount = (chunk == 256) ? 0 : (uint8_t)chunk;
            result = (uint32_t)ata_write28(ataData->ioBase, ataData->ctrlBase, ataData->slave, (uint32_t)(sector + totalWritten), regCount, wordsPerSector, buf16);
        }

        buf16 += result * wordsPerSector;
        totalWritten += result;

        if ((uint32_t)result != chunk) break;
    }

    return totalWritten;
}

void ATA_Destroy(Block_Device* dev)
{
    Memory_KernelFree(dev->data);
}

static char ATA_Type_Str[] = "ATA-DRIVE";

int ATA_GetDiskFromPCIDevice(const PCI_Device* pciDevice, Block_Device* blockDevice, bool primary, bool isSlave)
{
    const uint8_t slave = (isSlave ? 1 : 0);

    uint16_t ioBase;
    uint16_t ctrlBase;
    if (primary)
    {
        const bool native = (pciDevice->progIf & 0x01) != 0;
        ioBase = native ? (uint16_t)(pciDevice->bar[0] & 0xFFFFFFFC) : 0x1F0;
        ctrlBase = native ? (uint16_t)(pciDevice->bar[1] & 0xFFFFFFFC) : 0x3F6;
    }
    else // secondary
    {
        const bool native = (pciDevice->progIf & 0x04) != 0;
        ioBase = native ? (uint16_t)(pciDevice->bar[0] & 0xFFFFFFFC) : 0x170;
        ctrlBase = native ? (uint16_t)(pciDevice->bar[1] & 0xFFFFFFFC) : 0x376;
    }

    uint16_t identifyBuf[256];
    int result = ata_identify(ioBase, ctrlBase, slave, identifyBuf);
    if (result == ATA_IDENTIFY_ERROR)
        return ATA_ERROR;
    if (result != ATA_IDENTIFY_SUCCESS)
        return ATA_NOT_ATA;

    const bool supportsLBA48 = (identifyBuf[83] & (1 << 10)) != 0;

    ATA_Data* data = (ATA_Data*)Memory_KernelAllocate(sizeof(ATA_Data));
    if (!data) return ATA_ERROR;

    data->ioBase = ioBase;
    data->ctrlBase = ctrlBase;
    data->slave = slave;
    data->supportsLBA48 = supportsLBA48;

    memset(blockDevice->name, '\0', sizeof(blockDevice->name));
    ATA_ExtractModelName(identifyBuf, blockDevice->name);

    uint32_t wordsPerSector = 256;
    if ((identifyBuf[106] & 0xC000) == 0x4000)
    {
        if (identifyBuf[106] & 0x1000)
            wordsPerSector = ((uint32_t)identifyBuf[118] << 16) | identifyBuf[117];
    }

    blockDevice->sectorSize = wordsPerSector * 2;

    if (supportsLBA48)
    {
        uint64_t sectors = 0;
        sectors |= (uint64_t)identifyBuf[100];
        sectors |= (uint64_t)identifyBuf[101] << 16;
        sectors |= (uint64_t)identifyBuf[102] << 32;
        sectors |= (uint64_t)identifyBuf[103] << 48;
        blockDevice->sectorCount = sectors;
    }
    else
    {
        blockDevice->sectorSize = ((uint32_t)identifyBuf[61] << 16) | identifyBuf[60];
    }

    blockDevice->type = ATA_Type_Str;

    blockDevice->read = ATA_Read;
    blockDevice->write = ATA_Write;

    blockDevice->destroy = ATA_Destroy;

    blockDevice->data = data;

    return ATA_SUCCESS;
}
