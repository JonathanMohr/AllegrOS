#include "elf.h"

#include <stddef.h>
#include "../fat/fat.h"
#include "../io/io.h"
#include <memory.h>

ELFHeader headerBuffer;

ELFLoadInfo ELF_GetSize(Partition* partition, const char* path)
{
    ELFLoadInfo info = { 0xFFFFFFFFFFFFFFFF, 0 };

    FAT_File* fd = FAT_Open(partition, path);
    if (!fd)
    {
        IO_PutStringCritical("ELF_GetSize: Cannot open file!\n");
        return info;
    }

    if (FAT_Read(partition, fd, sizeof(ELFHeader), &headerBuffer) != sizeof(ELFHeader))
    {
        IO_PutStringCritical("ELF_GetSize: Cannot read header!\n");
        FAT_Close(fd);
        return info;
    }

    if (memcmp((const char*)headerBuffer.Magic, ELF_MAGIC, 4) != 0)
    {
        IO_PutStringCritical("ELF_GetSize: Invalid magic!\n");
        FAT_Close(fd);
        return info;
    }

    uint32_t programHeaderOffset = headerBuffer.ProgramHeaderTablePosition;
    uint32_t programHeaderSize = headerBuffer.ProgramHeaderTableEntrySize * headerBuffer.ProgramHeaderTableEntryCount;
    uint32_t programHeaderTableEntrySize = headerBuffer.ProgramHeaderTableEntrySize;
    uint32_t programHeaderTableEntryCount = headerBuffer.ProgramHeaderTableEntryCount;

    uint32_t remaining = programHeaderOffset - sizeof(ELFHeader);
    uint8_t buffer[512];
    while (remaining)
    {
        uint32_t toRead = (remaining < 512) ? remaining : 512;
        if (FAT_Read(partition, fd, toRead, buffer) != toRead)
        {
            IO_PutStringCritical("ELF_GetSize: Cannot seek to program headers!\n");
            FAT_Close(fd);
            return info;
        }
        remaining -= toRead;
    }

    uint64_t loadStart = 0xFFFFFFFFFFFFFFFF;
    uint64_t loadEnd = 0;

    for (uint32_t i = 0; i < programHeaderTableEntryCount; i++)
    {
        ELFProgramHeader progHeader;
        if (FAT_Read(partition, fd, sizeof(ELFProgramHeader), &progHeader) != sizeof(ELFProgramHeader))
        {
            IO_PutStringCritical("ELF_GetSize: Cannot read program headers!\n");
            FAT_Close(fd);
            return info;
        }

        if (progHeader.Type == ELF_PROGRAM_TYPE_LOAD)
        {
            if (progHeader.VirtualAddress < loadStart)
                loadStart = progHeader.VirtualAddress;

            uint64_t segmentEnd = progHeader.VirtualAddress + progHeader.MemorySize;
            if (segmentEnd > loadEnd)
                loadEnd = segmentEnd;
        }
    }

    FAT_Close(fd);

    if (loadStart <= loadEnd)
    {
        info.start = loadStart;
        info.size = loadEnd - loadStart;
    }
    else
    {
        info.start = 0;
        info.size = 0;
    }

    return info;
}

void FAT_Seek(FAT_File** fd, Partition* partition, const char* path, uint64_t offset)
{
    // ugly nasty seeking
    // TODO: proper seeking
    if (*fd) FAT_Close(*fd);
    *fd = FAT_Open(partition, path);
    if (!*fd)
    {
        IO_PutStringCritical("ELF_Load: Cannot open file for ugly seeking!\n");
        return;
    }

    uint64_t remaining = offset;
    uint8_t buffer[1024];
    while (remaining)
    {
        uint32_t toRead = (remaining < 1024) ? (uint32_t)remaining : 1024;
        if (FAT_Read(partition, *fd, toRead, buffer) != toRead)
        {
            IO_PutStringCritical("ELF_GetSize: Cannot read for ugly seeking!\n");
            FAT_Close(*fd);
            *fd = NULL;
            return;
        }
        remaining -= toRead;
    }
}

uint8_t* ELF_Load(Partition* partition, const char* path)
{
    FAT_File* fd = FAT_Open(partition, path);
    if (!fd)
    {
        IO_PutStringCritical("ELF_Load: Cannot open file!\n");
        return NULL;
    }

    if (FAT_Read(partition, fd, sizeof(ELFHeader), &headerBuffer) != sizeof(ELFHeader))
    {
        IO_PutStringCritical("ELF_Load: Cannot read header!\n");
        FAT_Close(fd);
        return NULL;
    }

    if (memcmp((const char*)headerBuffer.Magic, ELF_MAGIC, 4) != 0)
    {
        IO_PutStringCritical("ELF_Load: Invalid magic!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.Bitness != ELF_BITNESS_32BIT)
    {
        IO_PutStringCritical("ELF_Load: Invalid bitness!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.Endianness != ELF_ENDIANNESS_LITTLE)
    {
        IO_PutStringCritical("ELF_Load: Invalid endianness!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.ELFHeaderVersion != 1)
    {
        IO_PutStringCritical("ELF_Load: Invalid ELF header version!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.ELFVersion != 1)
    {
        IO_PutStringCritical("ELF_Load: Invalid ELF version!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.Type != ELF_TYPE_EXECUTABLE)
    {
        IO_PutStringCritical("ELF_Load: Not an executable!\n");
        FAT_Close(fd);
        return NULL;
    }
    if (headerBuffer.InstructionSet != ELF_INSTRUCTION_SET_X86)
    {
        IO_PutStringCritical("ELF_Load: Invalid instruction set!\n");
        FAT_Close(fd);
        return NULL;
    }

    uint8_t* entry = (uint8_t*)headerBuffer.ProgramEntryPosition;

    uint32_t programHeaderOffset = headerBuffer.ProgramHeaderTablePosition;
    uint32_t programHeaderSize = headerBuffer.ProgramHeaderTableEntrySize * headerBuffer.ProgramHeaderTableEntryCount;
    uint32_t programHeaderTableEntrySize = headerBuffer.ProgramHeaderTableEntrySize;
    uint32_t programHeaderTableEntryCount = headerBuffer.ProgramHeaderTableEntryCount;

    uint64_t currentOffset = programHeaderOffset;
    for (uint32_t i = 0; i < programHeaderTableEntryCount; i++)
    {
        ELFProgramHeader progHeader;
        FAT_Seek(&fd, partition, path, currentOffset);
        if (!fd) return NULL;
        if (FAT_Read(partition, fd, sizeof(ELFProgramHeader), &progHeader) != sizeof(ELFProgramHeader))
        {
            IO_PutStringCritical("ELF_GetSize: Cannot read program headers!\n");
            FAT_Close(fd);
            return NULL;
        }
        currentOffset += programHeaderTableEntrySize;

        if (progHeader.Type == ELF_PROGRAM_TYPE_LOAD)
        {
            uint8_t* virtAddress = (uint8_t*)progHeader.VirtualAddress;
            memset(virtAddress, 0, progHeader.MemorySize);

            FAT_Seek(&fd, partition, path, progHeader.Offset);
            if (!fd) return NULL;

            // read program
            if (FAT_Read(partition, fd, progHeader.FileSize, virtAddress) != progHeader.FileSize)
            {
                IO_PutStringCritical("ELF_GetSize: Cannot read program!\n");
                FAT_Close(fd);
                return NULL;
            }
        }
    }

    FAT_Close(fd);

    return entry;
}
