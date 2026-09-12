#include "elf.h"
#include "result.h"
#include <stdint.h>
#include <memory.h>

#include "../memory/memory.h"

struct ELF32_Header
{
    uint8_t magic[4];
    uint8_t class;
    uint8_t endianness;
    uint8_t fileVersion;
    uint8_t osabi;
    uint8_t abiVersion;
    uint8_t padding[7];

    uint16_t type;
    uint16_t machine;
    uint32_t version;

    uint32_t entry;
    uint32_t programHeaderTableOffset;
    uint32_t sectionHeaderTableOffset;

    uint32_t flags;

    uint16_t headerSize;
    uint16_t programHeaderSize;
    uint16_t programHeaderCount;
    uint16_t sectionHeaderSize;
    uint16_t sectionHeaderCount;
    uint16_t sectionNameStringTableIndex;
} __attribute__((packed));

struct ELF32_ProgramHeader
{
    uint32_t type;
    uint32_t offset;
    uint32_t virtualAddress;
    uint32_t physicalAddress;
    uint32_t fileSize;
    uint32_t memorySize;
    uint32_t flags;
    uint32_t align;
} __attribute__((packed));

#define ELF_HEADER_IDENT_CLASS_NONE ((uint8_t)0)
#define ELF_HEADER_IDENT_CLASS_32   ((uint8_t)1)
#define ELF_HEADER_IDENT_CLASS_64   ((uint8_t)2)

#define ELF_HEADER_IDENT_ENDIANNESS_NODE   ((uint8_t)0)
#define ELF_HEADER_IDENT_ENDIANNESS_LITTLE ((uint8_t)1)
#define ELF_HEADER_IDENT_ENDIANNESS_BIG    ((uint8_t)2)

#define ELF_HEADER_TYPE_NONE        ((uint16_t)0)
#define ELF_HEADER_TYPE_RELOCATABLE ((uint16_t)1)
#define ELF_HEADER_TYPE_EXECUTABLE  ((uint16_t)2)
#define ELF_HEADER_TYPE_DYNAMIC     ((uint16_t)3)
#define ELF_HEADER_TYPE_CORE        ((uint16_t)4)

#define ELF_HEADER_MACHINE_386      ((uint16_t)3)


#define ELF_PROGRAM_HEADER_TYPE_LOAD 1

#define ELF_PROGRAM_HEADER_FLAGS_EXECUTABLE 1
#define ELF_PROGRAM_HEADER_FLAGS_WRITABLE 2
#define ELF_PROGRAM_HEADER_FLAGS_READABLE 4


struct ELF_MappedPage
{
    uintptr_t virtualAddress;
    uintptr_t pageCount;
    Memory_Flags flags;
};

static void UnmapPages(AddressSpace* addressSpace, struct ELF_MappedPage* pages, uintptr_t upToEntry, uintptr_t upToPageInEntry)
{
    for (uintptr_t entryIndex = 0; entryIndex < upToEntry; entryIndex++)
        Memory_Unlink(addressSpace, pages[entryIndex].virtualAddress, pages[entryIndex].pageCount);

    if (upToPageInEntry > 0)
        Memory_Unlink(addressSpace, pages[upToEntry].virtualAddress, upToPageInEntry);
}

static ELF_Result MapPages(AddressSpace* addressSpace, struct ELF_MappedPage* pages, uintptr_t pageEntryCount)
{
    for (uintptr_t entryIndex = 0; entryIndex < pageEntryCount; entryIndex++)
    {
        if (Memory_LinkNew(addressSpace, pages[entryIndex].virtualAddress, pages[entryIndex].flags | MEMORY_WRITABLE, pages[entryIndex].pageCount) != MEMORY_SUCCESS)
        {
            UnmapPages(addressSpace, pages, entryIndex, 0);
            return ELF_ERROR_OUT_OF_MEMORY;
        }
    }

    return ELF_SUCCESS;
}

static ELF_Result FinalizePageFlags(AddressSpace* addressSpace, struct ELF_MappedPage* pages, uintptr_t pageEntryCount)
{
    for (uintptr_t entryIndex = 0; entryIndex < pageEntryCount; entryIndex++)
    {
        if (Memory_ChangeFlags(addressSpace, pages[entryIndex].virtualAddress, pages[entryIndex].pageCount, pages[entryIndex].flags) != MEMORY_SUCCESS)
            return ELF_ERROR_INTERNAL;
    }

    return ELF_SUCCESS;
}

ELF_Result ELF_Load(VFS_File* file, AddressSpace* addressSpace, bool currentAddressSpace, uintptr_t* outEntryPoint)
{
    struct ELF32_Header header;
    if (VFS_File_Read(file, sizeof(struct ELF32_Header), &header) != sizeof(struct ELF32_Header))
        return ELF_ERROR_IO;

    if (header.magic[0] != 0x7Fu || header.magic[1] != (uint8_t)'E' ||
        header.magic[2] != (uint8_t)'L' || header.magic[3] != (uint8_t)'F')
        return ELF_ERROR_FORMAT;

    if (header.fileVersion != 1)
        return ELF_ERROR_FORMAT;

    if (header.class != ELF_HEADER_IDENT_CLASS_32)
        return ELF_ERROR_WRONG_MACHINE;

    if (header.endianness != ELF_HEADER_IDENT_ENDIANNESS_LITTLE)
        return ELF_ERROR_WRONG_MACHINE;

    
    if (header.type != ELF_HEADER_TYPE_EXECUTABLE)
        return ELF_ERROR_TYPE;

    if (header.machine != ELF_HEADER_MACHINE_386)
        return ELF_ERROR_WRONG_MACHINE;

    if (header.version != 1)
        return ELF_ERROR_FORMAT;

    if (header.programHeaderSize < sizeof(struct ELF32_ProgramHeader))
        return ELF_ERROR_FORMAT;

    if (header.programHeaderCount == 0 || header.programHeaderTableOffset == 0)
        return ELF_ERROR_NOTHING_TO_LOAD;

    if (!VFS_File_Seek(file, header.programHeaderTableOffset))
        return ELF_ERROR_IO;

    struct ELF_MappedPage* pages = NULL;
    uintptr_t pageEntryCount = 0;
    uintptr_t pageEntryCapacity = 0;

    for (uint32_t i = 0; i < header.programHeaderCount; i++)
    {
        struct ELF32_ProgramHeader programHeader;
        if (VFS_File_Read(file, sizeof(struct ELF32_ProgramHeader), &programHeader) != sizeof(struct ELF32_ProgramHeader))
        {
            Memory_KernelFree(pages);
            return ELF_ERROR_IO;
        }
        
        if (header.programHeaderSize > sizeof(struct ELF32_ProgramHeader))
        {
            if (!VFS_File_Seek(file, header.programHeaderTableOffset + (i + 1) * header.programHeaderSize))
            {
                Memory_KernelFree(pages);
                return ELF_ERROR_IO;
            }
        }

        if (programHeader.type != ELF_PROGRAM_HEADER_TYPE_LOAD)
            continue;

        if (programHeader.fileSize > programHeader.memorySize)
        {
            Memory_KernelFree(pages);
            return ELF_ERROR_FORMAT;
        }

        if ((programHeader.virtualAddress % MEMORY_PAGE_SIZE) != (programHeader.offset % MEMORY_PAGE_SIZE))
        {
            Memory_KernelFree(pages);
            return ELF_ERROR_FORMAT;
        }

        const uintptr_t alignedStart = (programHeader.virtualAddress / MEMORY_PAGE_SIZE) * MEMORY_PAGE_SIZE;
        const uintptr_t alignedEnd = ((programHeader.virtualAddress + programHeader.memorySize + MEMORY_PAGE_SIZE - 1) / MEMORY_PAGE_SIZE) * MEMORY_PAGE_SIZE;
        const uintptr_t segmentPageCount = (alignedEnd - alignedStart) / MEMORY_PAGE_SIZE;

        Memory_Flags flags = MEMORY_USER;
        if (programHeader.flags & ELF_PROGRAM_HEADER_FLAGS_WRITABLE)
            flags |= MEMORY_WRITABLE;
        if (programHeader.flags & ELF_PROGRAM_HEADER_FLAGS_EXECUTABLE)
            flags |= MEMORY_EXECUTABLE;

        if (pageEntryCount == pageEntryCapacity)
        {
            uint32_t newCapacity = (pageEntryCapacity == 0) ? 4 : pageEntryCapacity * 2;
            struct ELF_MappedPage* newPages = Memory_KernelAllocate(newCapacity * sizeof(struct ELF_MappedPage));
            if (!newPages)
            {
                Memory_KernelFree(pages);
                return ELF_ERROR_OUT_OF_MEMORY;
            }

            for (uint32_t j = 0; j < pageEntryCount; j++)
                newPages[j] = pages[j];

            Memory_KernelFree(pages);
            pages = newPages;
            pageEntryCapacity = newCapacity;
        }

        pages[pageEntryCount].virtualAddress = alignedStart;
        pages[pageEntryCount].pageCount = segmentPageCount;
        pages[pageEntryCount].flags = flags;
        pageEntryCount++;
    }

    for (uint32_t i = 1; i < pageEntryCount; i++)
    {
        struct ELF_MappedPage key = pages[i];
        uint32_t j = i;
        while (j > 0 && pages[j - 1].virtualAddress > key.virtualAddress)
        {
            pages[j] = pages[j - 1];
            j--;
        }
        pages[j] = key;
    }

    uintptr_t boundaryCount = 0;
    uintptr_t* boundaries = Memory_KernelAllocate(2 * pageEntryCount * sizeof(uintptr_t));
    if (!boundaries)
    {
        Memory_KernelFree(pages);
        return ELF_ERROR_OUT_OF_MEMORY;
    }

    for (uintptr_t i = 0; i < pageEntryCount; i++)
    {
        boundaries[boundaryCount++] = pages[i].virtualAddress;
        boundaries[boundaryCount++] = pages[i].virtualAddress + pages[i].pageCount * MEMORY_PAGE_SIZE;
    }

    for (uintptr_t i = 1; i < boundaryCount; i++)
    {
        uintptr_t key = boundaries[i];
        uintptr_t j = i;
        while (j > 0 && boundaries[j - 1] > key)
        {
            boundaries[j] = boundaries[j - 1];
            j--;
        }
        boundaries[j] = key;
    }

    uintptr_t uniqueBoundaryCount = 0;
    for (uintptr_t i = 0; i < boundaryCount; i++)
    {
        if (i == 0 || boundaries[i] != boundaries[uniqueBoundaryCount - 1])
            boundaries[uniqueBoundaryCount++] = boundaries[i];
    }

    struct ELF_MappedPage* merged = NULL;
    uintptr_t mergedCount = 0;
    uintptr_t mergedCapacity = 0;

    for (uintptr_t b = 0; b + 1 < uniqueBoundaryCount; b++)
    {
        uintptr_t intervalStart = boundaries[b];
        uintptr_t intervalEnd = boundaries[b + 1];

        Memory_Flags intervalFlags = 0;
        bool covered = false;

        for (uintptr_t i = 0; i < pageEntryCount; i++)
        {
            uintptr_t segStart = pages[i].virtualAddress;
            uintptr_t segEnd = segStart + pages[i].pageCount * MEMORY_PAGE_SIZE;

            if (segStart <= intervalStart && segEnd >= intervalEnd)
            {
                intervalFlags |= pages[i].flags;
                covered = true;
            }
        }

        if (!covered)
            continue;

        if (mergedCount > 0 &&
            merged[mergedCount - 1].virtualAddress + merged[mergedCount - 1].pageCount * MEMORY_PAGE_SIZE == intervalStart &&
            merged[mergedCount - 1].flags == intervalFlags)
        {
            merged[mergedCount - 1].pageCount += (intervalEnd - intervalStart) / MEMORY_PAGE_SIZE;
            continue;
        }

        if (mergedCount == mergedCapacity)
        {
            uintptr_t newCapacity = (mergedCapacity == 0) ? 4 : mergedCapacity * 2;
            struct ELF_MappedPage* newMerged = Memory_KernelAllocate(newCapacity * sizeof(struct ELF_MappedPage));
            if (!newMerged)
            {
                Memory_KernelFree(merged);
                Memory_KernelFree(boundaries);
                Memory_KernelFree(pages);
                return ELF_ERROR_OUT_OF_MEMORY;
            }

            for (uintptr_t j = 0; j < mergedCount; j++)
                newMerged[j] = merged[j];

            Memory_KernelFree(merged);
            merged = newMerged;
            mergedCapacity = newCapacity;
        }

        merged[mergedCount].virtualAddress = intervalStart;
        merged[mergedCount].pageCount = (intervalEnd - intervalStart) / MEMORY_PAGE_SIZE;
        merged[mergedCount].flags = intervalFlags;
        mergedCount++;
    }

    Memory_KernelFree(boundaries);
    Memory_KernelFree(pages);

    pages = merged;
    pageEntryCount = mergedCount;

    const ELF_Result mapResult = MapPages(addressSpace, pages, pageEntryCount);
    if (mapResult != ELF_SUCCESS)
    {
        Memory_KernelFree(pages);
        return mapResult;
    }

    ELF_Result copyResult = ELF_SUCCESS;;

    if (currentAddressSpace)
    {
        if (!VFS_File_Seek(file, header.programHeaderTableOffset))
            copyResult = ELF_ERROR_IO;

        for (uint32_t i = 0; copyResult == ELF_SUCCESS && i < header.programHeaderCount; i++)
        {
            struct ELF32_ProgramHeader programHeader;
            if (VFS_File_Read(file, sizeof(struct ELF32_ProgramHeader), &programHeader) != sizeof(struct ELF32_ProgramHeader))
            {
                copyResult = ELF_ERROR_IO;
                break;
            }

            if (header.programHeaderSize > sizeof(struct ELF32_ProgramHeader))
            {
                if (!VFS_File_Seek(file, header.programHeaderTableOffset + (i + 1) * header.programHeaderSize))
                {
                    copyResult = ELF_ERROR_IO;
                    break;
                }
            }

            if (programHeader.type != ELF_PROGRAM_HEADER_TYPE_LOAD)
                continue;

            if (!VFS_File_Seek(file, programHeader.offset))
            {
                copyResult = ELF_ERROR_IO;
                break;
            }

            if (VFS_File_Read(file, programHeader.fileSize, (void*)programHeader.virtualAddress) != programHeader.fileSize)
            {
                copyResult = ELF_ERROR_IO;
                break;
            }

            memset((void*)(programHeader.virtualAddress + programHeader.fileSize), 0, programHeader.memorySize - programHeader.fileSize);
        }
    }
    else
    {
        uintptr_t scratchAddress;
        if (Memory_Kernel_AllocateVirtual(1, &scratchAddress) != MEMORY_SUCCESS)
        {
            UnmapPages(addressSpace, pages, pageEntryCount, 0);
            Memory_KernelFree(pages);
            return ELF_ERROR_OUT_OF_MEMORY;
        }

        copyResult = ELF_SUCCESS;

        if (!VFS_File_Seek(file, header.programHeaderTableOffset))
            copyResult = ELF_ERROR_IO;

        for (uint32_t i = 0; copyResult == ELF_SUCCESS && i < header.programHeaderCount; i++)
        {
            struct ELF32_ProgramHeader programHeader;
            if (VFS_File_Read(file, sizeof(struct ELF32_ProgramHeader), &programHeader) != sizeof(struct ELF32_ProgramHeader))
            {
                copyResult = ELF_ERROR_IO;
                break;
            }

            if (header.programHeaderSize > sizeof(struct ELF32_ProgramHeader))
            {
                if (!VFS_File_Seek(file, header.programHeaderTableOffset + (i + 1) * header.programHeaderSize))
                {
                    copyResult = ELF_ERROR_IO;
                    break;
                }
            }

            if (programHeader.type != ELF_PROGRAM_HEADER_TYPE_LOAD)
                continue;

            if (!VFS_File_Seek(file, programHeader.offset))
            {
                copyResult = ELF_ERROR_IO;
                break;
            }

            uintptr_t bytesRemaining = programHeader.fileSize;
            uintptr_t sourceVirtualAddress = programHeader.virtualAddress;

            while (bytesRemaining > 0)
            {
                uintptr_t pageVirtualAddress = (sourceVirtualAddress / MEMORY_PAGE_SIZE) * MEMORY_PAGE_SIZE;
                uintptr_t pageOffset = sourceVirtualAddress - pageVirtualAddress;
                uintptr_t chunkSize = MEMORY_PAGE_SIZE - pageOffset;
                if (chunkSize > bytesRemaining)
                    chunkSize = bytesRemaining;

                if (Memory_Link(MEMORY_KERNEL, scratchAddress, addressSpace, pageVirtualAddress, MEMORY_WRITABLE, 1) != MEMORY_SUCCESS)
                {
                    copyResult = ELF_ERROR_INTERNAL;
                    break;
                }

                if (VFS_File_Read(file, chunkSize, (void*)(scratchAddress + pageOffset)) != chunkSize)
                {
                    Memory_Unlink(MEMORY_KERNEL, scratchAddress, 1);
                    copyResult = ELF_ERROR_IO;
                    break;
                }

                Memory_Unlink(MEMORY_KERNEL, scratchAddress, 1);

                bytesRemaining -= chunkSize;
                sourceVirtualAddress += chunkSize;
            }

            if (copyResult != ELF_SUCCESS)
                break;

            uintptr_t zeroRemaining = programHeader.memorySize - programHeader.fileSize;
            uintptr_t zeroVirtualAddress = programHeader.virtualAddress + programHeader.fileSize;

            while (zeroRemaining > 0)
            {
                uintptr_t pageVirtualAddress = (zeroVirtualAddress / MEMORY_PAGE_SIZE) * MEMORY_PAGE_SIZE;
                uintptr_t pageOffset = zeroVirtualAddress - pageVirtualAddress;
                uintptr_t chunkSize = MEMORY_PAGE_SIZE - pageOffset;
                if (chunkSize > zeroRemaining)
                    chunkSize = zeroRemaining;

                if (Memory_Link(MEMORY_KERNEL, scratchAddress, addressSpace, pageVirtualAddress, MEMORY_WRITABLE, 1) != MEMORY_SUCCESS)
                {
                    copyResult = ELF_ERROR_INTERNAL;
                    break;
                }

                memset((void*)(scratchAddress + pageOffset), 0, chunkSize);

                Memory_Unlink(MEMORY_KERNEL, scratchAddress, 1);

                zeroRemaining -= chunkSize;
                zeroVirtualAddress += chunkSize;
            }
        }

        Memory_Kernel_FreeVirtual(scratchAddress, 1);
    }

    if (copyResult != ELF_SUCCESS)
    {
        UnmapPages(addressSpace, pages, pageEntryCount, 0);
        Memory_KernelFree(pages);
        return copyResult;
    }

    const ELF_Result finalizeResult = FinalizePageFlags(addressSpace, pages, pageEntryCount);
    if (finalizeResult != ELF_SUCCESS)
    {
        UnmapPages(addressSpace, pages, pageEntryCount, 0);
        Memory_KernelFree(pages);
        return finalizeResult;
    }

    Memory_KernelFree(pages);

    *outEntryPoint = header.entry;
    return ELF_SUCCESS;
}
