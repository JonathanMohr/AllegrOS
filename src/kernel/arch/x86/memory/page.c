#include "../../../memory/arch.h"
#include "../../../memory/memory.h"
#include "../../../panic/panic.h"
#include "../../../memory/physical.h"
#include "../../../lock.h"

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


static uphysptr_t currentAddressSpace;

void Arch_Memory_Initialize(Memory_Layout* layout, MemoryInfo* memoryInfo)
{
    layout->kernelSpaceEnd = 0xFFC00000;
    layout->kernelSpaceStart = 0xC0000000;

    layout->userSpaceStart = 0x00010000;
    layout->userSpaceEnd = 0xC0000000;

    layout->pageSize = PAGE_SIZE;

    currentAddressSpace = memoryInfo->initialAddressSpace;
}


static int allocated = 0;
static __attribute__((aligned(4096))) uint8_t scratchPageBuffer[4096];
static int allocated2 = 0;
static __attribute__((aligned(4096))) uint8_t scratchPageBuffer2[4096];

uintptr_t Arch_TemporaryMap(uphysptr_t physicalMapAddress)
{
    IRQ_PushDisable();

    if (allocated)
    {
        PanicMessage("Arch_TemporaryMap called without Arch_TemporaryUnmap\n");
        Panic();
    }

    const uintptr_t scratchVirtualAddr = (uintptr_t)scratchPageBuffer;
    const uint32_t dirIndex = scratchVirtualAddr >> 22;
    const uint32_t tableIndex = (scratchVirtualAddr >> 12) & 0x3FF;

    uint32_t* recursiveTable = (uint32_t*)(0xFFC00000 + dirIndex * 0x1000);

    allocated = 1;
    recursiveTable[tableIndex] = (physicalMapAddress & ~0xFFFu) | PAGEF_PRESENT | PAGEF_WRITABLE;
    x86_invlpg(scratchVirtualAddr);

    return scratchVirtualAddr;
}

void Arch_TemporaryUnmap(uintptr_t virtualMapAddress)
{
    (void)virtualMapAddress;
    allocated = 0;

    IRQ_PopDisable();
}

uintptr_t Arch_TemporaryMap2(uphysptr_t physicalMapAddress)
{
    IRQ_PushDisable();

    if (allocated2)
    {
        PanicMessage("Arch_TemporaryMap2 called without Arch_TemporaryUnmap2\n");
        Panic();
    }

    const uintptr_t scratchVirtualAddr = (uintptr_t)scratchPageBuffer2;
    const uint32_t dirIndex = scratchVirtualAddr >> 22;
    const uint32_t tableIndex = (scratchVirtualAddr >> 12) & 0x3FF;

    uint32_t* recursiveTable = (uint32_t*)(0xFFC00000 + dirIndex * 0x1000);

    allocated2 = 1;
    recursiveTable[tableIndex] = (physicalMapAddress & ~0xFFFu) | PAGEF_PRESENT | PAGEF_WRITABLE;
    x86_invlpg(scratchVirtualAddr);

    return scratchVirtualAddr;
}

void Arch_TemporaryUnmap2(uintptr_t virtualMapAddress)
{
    (void)virtualMapAddress;
    allocated2 = 0;

    IRQ_PopDisable();
}


static Memory_Result MirrorKernel(uint32_t* pageDirectory, uint32_t* newPageDirectory)
{
    for (uint16_t i = 768; i < 1023; i++)
    {
        const uint32_t oldEntry = pageDirectory[i];
        const uint32_t newEntry = newPageDirectory[i];

        if ((newEntry & PAGEF_PRESENT) && (oldEntry & PAGEF_PRESENT) && (oldEntry & ~0xFFFu) == (newEntry & ~0xFFFu))
            continue;

        if (newEntry & PAGEF_PRESENT)
        {
            const uphysptr_t newTable = newEntry & ~0xFFFu;
            if (Memory_Physical_GetPage(newTable) != MEMORY_SUCCESS)
                return MEMORY_ERROR_REFERENCE_LIMIT;
        }

        pageDirectory[i] = newEntry;

        if (oldEntry & PAGEF_PRESENT)
        {
            const uphysptr_t oldTable = oldEntry & ~0xFFFu;
            if (Memory_Physical_PutPage(oldTable) != MEMORY_SUCCESS)
                PanicMessageInfo("MirrorKernel", "Memory_Physical_PutPage(%q) failed\n", oldTable);
        }
    }

    return MEMORY_SUCCESS;
}

Memory_Result Arch_CreateAddressSpace(uphysptr_t* out)
{
    Memory_Result result;

    uphysptr_t pageDirectory;
    if ((result = Memory_Physical_NewPage(&pageDirectory)) != MEMORY_SUCCESS)
        return result;

    uint32_t* tmp = (uint32_t*)Arch_TemporaryMap(pageDirectory);

    memset(tmp, 0, PAGE_SIZE);

    if ((result = MirrorKernel(tmp, (uint32_t*)0xFFFFF000)) != MEMORY_SUCCESS)
    {
        Arch_TemporaryUnmap((uintptr_t)tmp);
        if (Memory_Physical_PutPage(pageDirectory) != MEMORY_SUCCESS)
            PanicMessageInfo("Arch_CreateAddressSpace", "Memory_Physical_PutPage(%q) failed\n", pageDirectory);
        return result;
    }

    tmp[PAGE_DIRECTORY_INDEX] = pageDirectory | PAGEF_PRESENT | PAGEF_WRITABLE;

    Arch_TemporaryUnmap((uintptr_t)tmp);

    *out = pageDirectory;

    return MEMORY_SUCCESS;
}

void Arch_DestroyAddressSpace(uphysptr_t addressSpace)
{
    uint32_t* directoryCopy = Memory_KernelAllocate(PAGE_SIZE);
    if (!directoryCopy)
    {
        PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_KernelAllocate failed\n", 0);
        return;
    }

    uint32_t* pageDirectory = (uint32_t*)Arch_TemporaryMap(addressSpace);
    memcpy(directoryCopy, pageDirectory, PAGE_SIZE);
    Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    for (uint16_t i = 0; i < 768; i++)
    {
        if ((directoryCopy[i] & PAGEF_PRESENT) == 0)
            continue;

        const uphysptr_t pageTablePhysical = directoryCopy[i] & ~0xFFFu;

        uint32_t* pageTable = (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
        for (uint16_t j = 0; j < 1024; j++)
        {
            if ((pageTable[j] & PAGEF_PRESENT) == 0)
                continue;

            const uphysptr_t dataPagePhysical = pageTable[j] & ~0xFFFu;
            if (Memory_Physical_PutPage(dataPagePhysical) != MEMORY_SUCCESS)
                PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_Physical_PutPage(%q) failed\n", dataPagePhysical);
        }
        Arch_TemporaryUnmap((uintptr_t)pageTable);

        if (Memory_Physical_PutPage(pageTablePhysical) != MEMORY_SUCCESS)
            PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_Physical_PutPage(%q) failed\n", pageTablePhysical);
    }

    for (uint16_t i = 768; i < 1023; i++)
    {
        if ((directoryCopy[i] & PAGEF_PRESENT) == 0)
            continue;

        const uphysptr_t pageTablePhysical = directoryCopy[i] & ~0xFFFu;
        if (Memory_Physical_PutPage(pageTablePhysical) != MEMORY_SUCCESS)
            PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_Physical_PutPage(%q) failed\n", pageTablePhysical);
    }

    Memory_KernelFree(directoryCopy);

    if (Memory_Physical_PutPage(addressSpace) != MEMORY_SUCCESS)
        PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_Physical_PutPage(%q) failed\n", addressSpace);
}

Memory_Result Arch_MapPage(uphysptr_t addressSpace, uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags)
{
    uint32_t pflags = PAGEF_PRESENT;
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

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFFu;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    if (pageTable[tableIndex] & PAGEF_PRESENT)
    {
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);
        return MEMORY_ERROR_ALREADY_MAPPED;
    }

    pageTable[tableIndex] = (physicalAddr & ~0xFFFu) | pflags;

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

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFFu;

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
        // TODO: Kernel Sync
        
        uint32_t* pageDirectoryForClear = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
        pageDirectoryForClear[dirIndex] = 0;
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectoryForClear);

        if (current)
            x86_reload_cr3();

        if (Memory_Physical_PutPage(pageTablePhysical) != MEMORY_SUCCESS)
            PanicMessageInfo("Arch_DestroyAddressSpace", "Memory_Physical_PutPage(%q) failed\n", pageTablePhysical);
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

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFFu;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    const uint32_t tableEntry = pageTable[tableIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if ((tableEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    uphysptr_t pagePhysical = tableEntry & ~0xFFFu;

    *outPhysicalAddr = pagePhysical + offset;

    return MEMORY_SUCCESS;
}

Memory_Result Arch_ChangeFlags(uphysptr_t addressSpace, uintptr_t virtualAddr, Memory_Flags flags)
{
    uint32_t pflags = PAGEF_PRESENT;
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
    const uint32_t dirEntry = pageDirectory[dirIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    if ((dirEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFFu;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    const uint32_t tableEntry = pageTable[tableIndex];

    if ((tableEntry & PAGEF_PRESENT) == 0)
    {
        if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);
        return MEMORY_ERROR_NOT_MAPPED;
    }

    const uint32_t managedMask = PAGEF_PRESENT | PAGEF_WRITABLE | PAGEF_USER | PAGEF_GLOBAL;
    pageTable[tableIndex] = (tableEntry & ~managedMask) | pflags;
    
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if (current)
        x86_invlpg(virtualAddr);

    return MEMORY_SUCCESS;
}

Memory_Result Arch_GetFlags(uphysptr_t addressSpace, uintptr_t virtualAddr, Memory_Flags* out)
{
    const bool current = addressSpace == currentAddressSpace;

    const uint32_t dirIndex   = virtualAddr >> 22;
    const uint32_t tableIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t* pageDirectory = current ? (uint32_t*)0xFFFFF000 : (uint32_t*)Arch_TemporaryMap(addressSpace);
    const uint32_t dirEntry = pageDirectory[dirIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageDirectory);

    if ((dirEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    const uphysptr_t pageTablePhysical = dirEntry & ~0xFFFu;

    uint32_t* pageTable = current ? (uint32_t*)(0xFFC00000 + dirIndex * 0x1000) : (uint32_t*)Arch_TemporaryMap(pageTablePhysical);
    const uint32_t tableEntry = pageTable[tableIndex];
    if (!current) Arch_TemporaryUnmap((uintptr_t)pageTable);

    if ((tableEntry & PAGEF_PRESENT) == 0)
        return MEMORY_ERROR_NOT_MAPPED;

    Memory_Flags flags = 0;
    if (tableEntry & PAGEF_WRITABLE)
        flags |= MEMORY_WRITABLE;
    if (tableEntry & PAGEF_USER)
        flags |= MEMORY_USER;
    if (tableEntry & PAGEF_GLOBAL)
        flags |= MEMORY_GLOBAL;

    *out = flags;
    return MEMORY_SUCCESS;
}


Memory_Result Arch_SyncKernel(uphysptr_t addressSpace, uphysptr_t newAddressSpace)
{
    const bool addressSpaceCurrent = addressSpace == currentAddressSpace;
    const bool newAddressSpaceCurrent = newAddressSpace == currentAddressSpace;

    if (addressSpaceCurrent == newAddressSpaceCurrent)
        return MEMORY_SUCCESS;

    if (addressSpaceCurrent)
    {
        uint32_t* newPageDirectory = (uint32_t*)Arch_TemporaryMap(newAddressSpace);
        const Memory_Result result = MirrorKernel((uint32_t*)0xFFFFF000, newPageDirectory);
        Arch_TemporaryUnmap((uintptr_t)newPageDirectory);

        if (result != MEMORY_SUCCESS)
            return result;

        x86_reload_cr3();
    }
    else if (newAddressSpaceCurrent)
    {
        uint32_t* currentPageDirectory = (uint32_t*)Arch_TemporaryMap(addressSpace);
        const Memory_Result result = MirrorKernel(currentPageDirectory, (uint32_t*)0xFFFFF000);
        Arch_TemporaryUnmap((uintptr_t)currentPageDirectory);

        if (result != MEMORY_SUCCESS)
            return result;
    }
    else
    {
        uint32_t kernelEntries[1023 - 768];

        uint32_t* newPageDirectory = (uint32_t*)Arch_TemporaryMap(newAddressSpace);
        memcpy(kernelEntries, &newPageDirectory[768], sizeof(kernelEntries));
        Arch_TemporaryUnmap((uintptr_t)newPageDirectory);

        uint32_t* currentPageDirectory = (uint32_t*)Arch_TemporaryMap(addressSpace);

        for (uint16_t i = 0; i < 1023 - 768; i++)
        {
            const uint32_t oldEntry = currentPageDirectory[768 + i];
            const uint32_t newEntry = kernelEntries[i];

            if ((newEntry & PAGEF_PRESENT) && (oldEntry & PAGEF_PRESENT) && (oldEntry & ~0xFFFu) == (newEntry & ~0xFFFu))
                continue;

            if (newEntry & PAGEF_PRESENT)
            {
                const uphysptr_t newTable = newEntry & ~0xFFFu;
                if (Memory_Physical_GetPage(newTable) != MEMORY_SUCCESS)
                {
                    Arch_TemporaryUnmap((uintptr_t)currentPageDirectory);
                    return MEMORY_ERROR_REFERENCE_LIMIT;
                }
            }

            currentPageDirectory[768 + i] = newEntry;

            if (oldEntry & PAGEF_PRESENT)
            {
                const uphysptr_t oldTable = oldEntry & ~0xFFFu;
                if (Memory_Physical_PutPage(oldTable) != MEMORY_SUCCESS)
                    PanicMessageInfo("Arch_SyncKernel", "Memory_Physical_PutPage(%q) failed\n", oldTable);
            }
        }

        Arch_TemporaryUnmap((uintptr_t)currentPageDirectory);
    }

    return MEMORY_SUCCESS;
}


void Arch_SwitchAddressSpace(uphysptr_t addressSpace)
{
    if (addressSpace == currentAddressSpace)
        return;

    currentAddressSpace = addressSpace;
    x86_load_cr3(addressSpace);
}
