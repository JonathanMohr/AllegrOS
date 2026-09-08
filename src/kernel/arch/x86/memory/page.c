#include "../../../newmemory/arch.h"
#include "../../../panic/panic.h"
#include "../../../newmemory/physical.h"

#include "paging.h"
#include <memory.h>


#define PAGE_DIRECTORY_INDEX 1023

#define PAGE_SIZE 4096

#define PAGEF_PRESENT        0x001
#define PAGEF_WRITABLE       0x002
#define PAGEF_USER           0x004
#define PAGEF_WRITE_THROUGH  0x008
#define PAGEF_CACHE_DISABLED 0x010
#define PAGEF_ACCESSED       0x020
#define PAGEF_DIRTY          0x040
#define PAGEF_SIZE           0x080
#define PAGEF_GLOBAL         0x100


void Arch_Initialize(Memory_Layout* layout)
{
    layout->kernelSpaceEnd = 0xFFC00000;
    layout->kernelSpaceStart = 0xC0000000;

    layout->userSpaceStart = 0x00010000;
    layout->userSpaceEnd = 0xC0000000;

    layout->pageSize = PAGE_SIZE;
}


static int allocated = 0;
static __attribute__((aligned(4096))) uint8_t scratchPageBuffer[4096];

uintptr_t Arch_TemporaryMap(uphysptr_t physicalMapAddress)
{
    if (allocated)
    {
        PanicMessage("Arch_TemporaryMap called with Arch_TemporaryUnmap\n");
        Panic();
    }

    const uintptr_t scratchVirtualAddr = (uintptr_t)scratchPageBuffer;
    const uint32_t dirIndex = scratchVirtualAddr >> 22;
    const uint32_t tableIndex = (scratchVirtualAddr >> 12) & 0x3FF;

    uint32_t* recursiveTable = (uint32_t*)(0xFFC00000 + dirIndex * 0x1000);

    allocated = 1;
    recursiveTable[tableIndex] = (physicalMapAddress & ~0xFFF) | PAGEF_PRESENT | PAGEF_WRITABLE;
    x86_invlpg(scratchVirtualAddr);

    return scratchVirtualAddr;
}

void Arch_TemporaryUnmap(uintptr_t virtualMapAddress)
{
    (void)virtualMapAddress;
    allocated = 0;
}

static void MirrorKernel(uint32_t* pageDirectory)
{
    uint32_t* currentPageDirectory = (uint32_t*)0xFFFFF000;

    for (uint16_t i = 768; i < 1023; i++)
        pageDirectory[i] = currentPageDirectory[i];
}

Memory_Result Arch_CreateAddressSpace(uphysptr_t* out)
{
    Memory_Result result;

    uphysptr_t pageDirectory;
    if ((result = Memory_Physical_NewPage(&pageDirectory)) != MEMORY_SUCCESS)
        return result;

    uint32_t* tmp = (uint32_t*)Arch_TemporaryMap(pageDirectory);

    memset(tmp, 0, PAGE_SIZE);

    MirrorKernel(tmp);

    tmp[PAGE_DIRECTORY_INDEX] = pageDirectory | PAGEF_PRESENT | PAGEF_WRITABLE;

    Arch_TemporaryUnmap((uintptr_t)tmp);

    *out = pageDirectory;

    return MEMORY_SUCCESS;
}

void Arch_DestroyAddressSpace(uphysptr_t addressSpace)
{
    uint32_t* pageDirectory = (uint32_t*)Arch_TemporaryMap(addressSpace);

    for (uint16_t i = 0; i < 768; i++)
    {
        if ((pageDirectory[i] & PAGEF_PRESENT) == 0)
            continue;

        const uphysptr_t pageTablePhysical = pageDirectory[i] & ~0xFFF;

        uint32_t* pageTable = (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
        for (uint16_t j = 0; j < 1024; j++)
        {
            if ((pageTable[j] & PAGEF_PRESENT) == 0)
                continue;

            const uphysptr_t dataPagePhysical = pageTable[j] & ~0xFFF;
            (void)Memory_Physical_PutPage(dataPagePhysical);
        }
        Arch_TemporaryUnmap((uintptr_t)pageTable);

        (void)Memory_Physical_PutPage(pageTablePhysical);
    }

    Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    (void)Memory_Physical_PutPage(addressSpace);
}

static uphysptr_t currentAddressSpace = 0; // TODO

Memory_Result Arch_MapPage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags)
{
    uint32_t pflags = 0;
    if (flags & MEMORY_WRITABLE)
        pflags |= PAGEF_WRITABLE;
    if (flags & MEMORY_USER)
        pflags |= PAGEF_USER;
    if (flags & MEMORY_GLOBAL)
        pflags |= PAGEF_GLOBAL;

    const bool current = addressSpace == currentAddressSpace;

    const uint32_t dirIndex = virtualAddr >> 22;
    const uint32_t tableIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t* pageDirectory = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
    uint32_t dirEntry = pageDirectory[dirIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    if ((dirEntry & PAGEF_PRESENT) == 0)
    {
        Memory_Result result;

        uphysptr_t newTablePhysical;
        if ((result = Memory_Physical_NewPage(&newTablePhysical)) != MEMORY_SUCCESS)
            return result;

        uint32_t* newTable = (uint32_t*)Arch_TemporaryMap(newTablePhysical);
        memset(newTable, 0, PAGE_SIZE);
        Arch_TemporaryUnmap((uintptr_t)newTable);

        dirEntry = newTablePhysical | PAGEF_PRESENT | PAGEF_WRITABLE;
        if (flags & MEMORY_USER)
            dirEntry |= PAGEF_USER;
        
        if (!current)
            pageDirectory = (uint32_t*)Arch_TemporaryMap(addressSpace);

        pageDirectory[dirIndex] = dirEntry;

        if (!current)
            Arch_TemporaryUnmap((uintptr_t)pageDirectory);
        else
            x86_reload_cr3();
    }

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFF;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    if (pageTable[tableIndex] & PAGEF_PRESENT)
    {
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);
        return MEMORY_ERROR_ALREADY_MAPPED;
    }

    pageTable[tableIndex] = (physicalAddr & ~0xFFF) | pflags;

    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if (current)
        x86_invlpg(virtualAddr);

    return MEMORY_SUCCESS;
}

Memory_Result Arch_UnmapPage(uphysptr_t addressSpace, uintptr_t virtualAddr)
{
    const bool current = addressSpace == currentAddressSpace;

    const uint32_t dirIndex = virtualAddr >> 22;
    const uint32_t tableIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t* pageDirectory = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
    const uint32_t dirEntry = pageDirectory[dirIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    if ((dirEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFF;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    if ((pageTable[tableIndex] & PAGEF_PRESENT) == 0)
    {
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);
        return MEMORY_ERROR_NOT_MAPPED;
    }

    pageTable[tableIndex] = 0;

    bool tableEmpty = true;
    for (uint32_t i = 0; i < 1024; i++)
    {
        if (pageTable[i] & PAGEF_PRESENT)
        {
            tableEmpty = false;
            break;
        }
    }

    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if (current)
        x86_invlpg(virtualAddr);

    if (tableEmpty)
    {
        uint32_t* pageDirectoryForClear = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
        pageDirectoryForClear[dirIndex] = 0;
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectoryForClear);

        if (current)
            x86_reload_cr3();

        (void)Memory_Physical_PutPage(pageTablePhysical);
    }

    return MEMORY_SUCCESS;
}

Memory_Result Arch_TranslatePage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t* outPhysicalAddr)
{
    const bool current = addressSpace == currentAddressSpace;

    const uint32_t dirIndex = virtualAddr >> 22;
    const uint32_t tableIndex = (virtualAddr >> 12) & 0x3FF;
    const uint32_t offset = virtualAddr & 0xFFF;

    uint32_t* pageDirectory = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
    const uint32_t dirEntry = pageDirectory[dirIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    if ((dirEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFF;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    const uint32_t tableEntry = pageTable[tableIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if ((tableEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    uphysptr_t pagePhysical = tableEntry & ~0xFFF;

    *outPhysicalAddr = pagePhysical + offset;

    return MEMORY_SUCCESS;
}

void Arch_SwitchAddressSpace(uphysptr_t addressSpace)
{
    if (addressSpace == currentAddressSpace)
        return;

    currentAddressSpace = addressSpace;
    x86_load_cr3(addressSpace);
}
