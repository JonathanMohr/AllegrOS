#include "elf.h"
#include "fat.h"
#include "memdefs.h"
#include "minmax.h"
#include "stdio.h"
#include <core/memory/memory.h>

uint64_t ELF_Size(Partition* part, const char* path)
{
    uint8_t* headerBuffer = MEMORY_ELF_ADDR;
    uint32_t read;

    FAT_File* fd = FAT_Open(part, path);
    if (!fd) {
        printf("ELF Size error: cannot open file\n");
        return 0;
    }

    if ((read = FAT_Read(part, fd, sizeof(ELFHeader), headerBuffer)) != sizeof(ELFHeader))
    {
        printf("ELF Size error: cannot read header\n");
        FAT_Close(fd);
        return 0;
    }

    ELFHeader* header = (ELFHeader*)headerBuffer;
    if (memcmp(header->Magic, ELF_MAGIC, 4) != 0)
    {
        printf("ELF Size error: invalid magic\n");
        FAT_Close(fd);
        return 0;
    }

    // Parameter aus Header lesen
    uint32_t programHeaderOffset = header->ProgramHeaderTablePosition;
    uint32_t programHeaderSize = header->ProgramHeaderTableEntrySize * header->ProgramHeaderTableEntryCount;
    uint32_t programHeaderTableEntrySize = header->ProgramHeaderTableEntrySize;
    uint32_t programHeaderTableEntryCount = header->ProgramHeaderTableEntryCount;

    // HeaderBuffer neu füllen mit Programmheadern
    if (FAT_Read(part, fd, programHeaderOffset - sizeof(ELFHeader), headerBuffer) != programHeaderOffset - sizeof(ELFHeader))
    {
        printf("ELF Size error: cannot seek to program headers\n");
        FAT_Close(fd);
        return 0;
    }
    if ((read = FAT_Read(part, fd, programHeaderSize, headerBuffer)) != programHeaderSize)
    {
        printf("ELF Size error: cannot read program headers\n");
        FAT_Close(fd);
        return 0;
    }
    FAT_Close(fd);

    uint64_t loadSize = 0;
    for (uint32_t i = 0; i < programHeaderTableEntryCount; i++)
    {
        ELFProgramHeader* progHeader = (ELFProgramHeader*)(headerBuffer + i * programHeaderTableEntrySize);
        if (progHeader->Type == ELF_PROGRAM_TYPE_LOAD) {
            loadSize += progHeader->MemorySize;
        }
    }

    return loadSize;
}

uint32_t ELF_Read(Partition* part, const char* path, void** entryPoint)
{
    uint8_t* headerBuffer = MEMORY_ELF_ADDR;
    uint8_t* loadBuffer = MEMORY_LOAD_KERNEL;
    uint32_t filePos = 0;
    uint32_t read;

    // Read header
    FAT_File* fd = FAT_Open(part, path);
    if ((read = FAT_Read(part, fd, sizeof(ELFHeader), headerBuffer)) != sizeof(ELFHeader))
    {
        printf("ELF Load error!\n");
        return 0;
    }
    filePos += read;

    // validate header
    bool ok = true;
    ELFHeader* header = (ELFHeader*)headerBuffer;
    ok = ok && (memcmp(header->Magic, ELF_MAGIC, 4) == 0);
    ok = ok && (header->Bitness == ELF_BITNESS_32BIT);
    ok = ok && (header->Endianness == ELF_ENDIANNESS_LITTLE);
    ok = ok && (header->ELFHeaderVersion == 1);
    ok = ok && (header->ELFVersion == 1);
    ok = ok && (header->Type == ELF_TYPE_EXECUTABLE);
    ok = ok && (header->InstructionSet == ELF_INSTRUCTION_SET_X86);

    *entryPoint = (void*)header->ProgramEntryPosition;

    // load program header
    uint32_t programHeaderOffset = header->ProgramHeaderTablePosition;
    uint32_t programHeaderSize = header->ProgramHeaderTableEntrySize * header->ProgramHeaderTableEntryCount;
    uint32_t programHeaderTableEntrySize = header->ProgramHeaderTableEntrySize;
    uint32_t programHeaderTableEntryCount = header->ProgramHeaderTableEntryCount;

    filePos += FAT_Read(part, fd, programHeaderOffset - filePos, headerBuffer);
    if ((read = FAT_Read(part, fd, programHeaderSize, headerBuffer)) != programHeaderSize)
    {
        printf("ELF Load error!\n");
        return 0;
    }
    filePos += read;
    FAT_Close(fd);

    uint32_t loadSize = 0;

    // parse program header entries
    for (uint32_t i = 0; i < programHeaderTableEntryCount; i++)
    {
        ELFProgramHeader* progHeader = (ELFProgramHeader*)(headerBuffer + i * programHeaderTableEntrySize);
        if (progHeader->Type == ELF_PROGRAM_TYPE_LOAD) {
            loadSize += progHeader->MemorySize;
            // TODO: validate that the program doesn't overwrite the stage2
            uint8_t* virtAddress = (uint8_t*)progHeader->VirtualAddress;
            memset(virtAddress, 0, progHeader->MemorySize);
            
            // ugly nasty seeking
            // TODO: proper seeking
            fd = FAT_Open(part, path);
            while (progHeader->Offset > 0) 
            {
                uint32_t shouldRead = min(progHeader->Offset, MEMORY_LOAD_SIZE);
                read = FAT_Read(part, fd, shouldRead, loadBuffer);
                if (read != shouldRead) {
                    printf("ELF Load error!\n");
                    return 0;
                }
                progHeader->Offset -= read;
            }

            // read program
            while (progHeader->FileSize > 0) 
            {
                uint32_t shouldRead = min(progHeader->FileSize, MEMORY_LOAD_SIZE);
                read = FAT_Read(part, fd, shouldRead, loadBuffer);
                if (read != shouldRead) {
                    printf("ELF Load error!\n");
                    return 0;
                }
                progHeader->FileSize -= read;

                memcpy(virtAddress, loadBuffer, read);
                virtAddress += read;
            }

            FAT_Close(fd);
        }
    }

    return loadSize;
}