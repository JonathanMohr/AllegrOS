#include "elf.h"
#include "../../memory/memory.h"
#include "../fat/fat.h"
#include "../../debug.h"
#include <core/memory/memory.h>
#include <core/minmax.h>

#define MEMORY_LOAD_SIZE 0x10000

uint32_t ELF_Read(Partition* part, const char* path, void** entryPoint)
{
    uint8_t* headerBuffer = (uint8_t*)memory_Allocate(sizeof(ELFHeader), 1);
    uint8_t* loadBuffer = (uint8_t*)memory_Allocate(MEMORY_LOAD_SIZE, 1);
    uint32_t filePos = 0;
    uint32_t read;

    // Read header
    FAT_File* fd = FAT_Open(part, path);
    if ((read = FAT_Read(part, fd, sizeof(ELFHeader), headerBuffer)) != sizeof(ELFHeader))
    {
        log_err("ELF", "Load error!");
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

    uint8_t* programHeaderBuffer = (uint8_t*)memory_Allocate(programHeaderSize, 1);

    uint8_t tmp[512];
    uint32_t toSkip = programHeaderOffset - filePos;
    while (toSkip > 0) {
        uint32_t chunk = toSkip > sizeof(tmp) ? sizeof(tmp) : toSkip;
        int r = FAT_Read(part, fd, chunk, tmp);
        if (r != chunk) {
            log_err("ELF", "Load error while skipping!");
            FAT_Close(fd);
            return 0;
        }
        toSkip -= r;
        filePos += r;
    }

    if ((read = FAT_Read(part, fd, programHeaderSize, programHeaderBuffer)) != programHeaderSize)
    {
        log_err("ELF", "Load error!");
        return 0;
    }
    filePos += read;
    FAT_Close(fd);

    uint32_t loadSize = 0;

    // parse program header entries
    for (uint32_t i = 0; i < programHeaderTableEntryCount; i++)
    {
        ELFProgramHeader* progHeader = (ELFProgramHeader*)(programHeaderBuffer + i * programHeaderTableEntrySize);
        if (progHeader->Type == ELF_PROGRAM_TYPE_LOAD)
        {
            loadSize += progHeader->MemorySize;
            uint8_t* virtAddress = (uint8_t*)progHeader->VirtualAddress;

            //TODO: change that it's always user
            if (!specific_Allocate(progHeader->MemorySize, virtAddress, true))
            {
                log_err("ELF", "Memory allocation failed for segment!");
                return 0;
            }

            memset(virtAddress, 0, progHeader->MemorySize);

            // ugly nasty seeking
            // TODO: proper seeking
            fd = FAT_Open(part, path);
            if (!fd) {
                log_err("ELF", "Failed to reopen file for segment data!");
                return 0;
            }

            uint32_t segmentOffset = progHeader->Offset;
            uint8_t tmp[512];
            while (segmentOffset > 0) {
                uint32_t chunk = segmentOffset > sizeof(tmp) ? sizeof(tmp) : segmentOffset;
                int r = FAT_Read(part, fd, chunk, tmp);
                if (r != chunk) {
                    log_err("ELF", "Load error while skipping segment data!");
                    FAT_Close(fd);
                    return 0;
                }
                segmentOffset -= r;
            }

            // read program
            uint32_t segmentFileSize = progHeader->FileSize;
            while (segmentFileSize > 0) 
            {
                uint32_t shouldRead = min(segmentFileSize, MEMORY_LOAD_SIZE);
                read = FAT_Read(part, fd, shouldRead, loadBuffer);
                if (read != shouldRead) {
                    log_err("ELF", "Load error!");
                    FAT_Close(fd);
                    return 0;
                }
                segmentFileSize -= read;

                memcpy(virtAddress, loadBuffer, read);
                virtAddress += read;
            }

            FAT_Close(fd);
        }
    }

    memory_Free(headerBuffer);
    memory_Free(loadBuffer);
    memory_Free(programHeaderBuffer);

    return loadSize;
}