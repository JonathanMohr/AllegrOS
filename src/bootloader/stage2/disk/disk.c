#include "disk.h"

#include "../x86/x86.h"
#include "../io/io.h"

uint8_t Disk_ReadBuffer[512 * 127];

bool Disk_Initialize(Disk* disk, uint8_t driveNumber)
{
    disk->id = driveNumber;

    if (((uintptr_t)Disk_ReadBuffer + sizeof(Disk_ReadBuffer)) >= 0x100000)
    {
        IO_PutStringCritical("Disk_ReadBuffer above 1MB limit for real mode!\n");
        return false;
    }

    return true;
}

bool Disk_ReadSectors(Disk* disk, uint64_t lba, uint64_t sectors, void* buffer)
{
    uintptr_t out = (uintptr_t)buffer;

    IO_PrintFormat(dbgout, "buffer: %p\n", out);

    while (sectors != 0)
    {
        uint8_t sectorsToRead = 127;
        if (sectors < 127) sectorsToRead = (uint8_t)sectors;

        Disk_AddressPacket dap;
        dap.size = 16;
        dap.reserved = 0;
        dap.count = sectorsToRead;
        dap.buffer_offset = (uint16_t)((uintptr_t)Disk_ReadBuffer & 0xF);
        dap.buffer_segment = (uint16_t)(((uintptr_t)Disk_ReadBuffer >> 4) & 0xFFFF);
        dap.lba = lba;

        if (!Disk_ReadRaw((uint32_t)disk->id, &dap))
            return false;

        memcpy((uint8_t*)out, Disk_ReadBuffer, sectorsToRead * 512);

        lba += sectorsToRead;
        sectors -= sectorsToRead;
        out += (uintptr_t)sectorsToRead * 512;
    }

    for (int i = 0; i < 512; i++)
    {
        IO_PrintFormat(dbgout, "%uxb%c", Disk_ReadBuffer[i], ((i + 1) % 16 == 0) ? '\n' : ' ');
    }

    for (int i = 0; i < 512; i++)
    {
        IO_PrintFormat(dbgout, "%uxb%c", ((uint8_t*)buffer)[i], ((i + 1) % 16 == 0) ? '\n' : ' ');
    }

    return true;
}
