#include "memory.h"
#include <stddef.h>

#include "arch.h"
#include "kernel.h"
#include "physical.h"
#include "result.h"

#include "../panic/panic.h"

struct AddressSpace
{
    uphysptr_t addressSpace;

    struct AddressSpace* before;
    struct AddressSpace* next;

    uint32_t references;
};

static AddressSpace* currentAddressSpace = NULL;
static AddressSpace* headAddressSpace = NULL;

Memory_Layout memoryLayout;
static uphysptr_t initialAddressSpace;
static uintptr_t kernelStart;


Memory_Result Memory_Initialize(MemoryInfo* memoryInfo)
{
    Memory_Result result;

    Arch_Memory_Initialize(&memoryLayout, memoryInfo);

    if ((result = Memory_Physical_Initialize(memoryInfo)) != MEMORY_SUCCESS)
        return result;

    if ((result = Memory_KernelVirtual_Initialize(memoryInfo)) != MEMORY_SUCCESS)
        return result;

    initialAddressSpace = memoryInfo->initialAddressSpace;
    kernelStart = (uintptr_t)memoryInfo->startOfFree;

    return MEMORY_SUCCESS;
}


Memory_Result Memory_MapPageKernel(uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags)
{
    const uphysptr_t current = currentAddressSpace ? currentAddressSpace->addressSpace : initialAddressSpace;

    Memory_Result result;
    if ((result = Arch_MapPage(current, virtualAddr, physicalAddr, flags)) != MEMORY_SUCCESS)
        return result;

    AddressSpace* currentAS = headAddressSpace;
    while (currentAS)
    {
        if (currentAS->addressSpace == current) continue;
        Memory_Result r = Arch_SyncKernel(currentAS->addressSpace, current);
        (void)r;

        // TODO: Check

        currentAS = currentAS->next;
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_UnmapPageKernel(uintptr_t virtualAddr)
{
    const uphysptr_t current = currentAddressSpace ? currentAddressSpace->addressSpace : initialAddressSpace;

    Memory_Result result;
    if ((result = Arch_UnmapPage(current, virtualAddr)) != MEMORY_SUCCESS)
        return result;

    AddressSpace* currentAS = headAddressSpace;
    while (currentAS)
    {
        if (currentAS->addressSpace == current) continue;
        Memory_Result r = Arch_SyncKernel(currentAS->addressSpace, current);
        (void)r;

        // TODO: Check

        currentAS = currentAS->next;
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_TranslateKernel(uintptr_t virtualAddr, uphysptr_t* out)
{
    const uphysptr_t current = currentAddressSpace ? currentAddressSpace->addressSpace : initialAddressSpace;
    return Arch_TranslatePage(current, virtualAddr, out);
}


static bool checkAddresses(AddressSpace* addressSpace, uintptr_t address, uintptr_t count)
{
    const uintptr_t start = (addressSpace == MEMORY_KERNEL)
                            ? kernelStart : memoryLayout.userSpaceStart;

    const uintptr_t end = (addressSpace == MEMORY_KERNEL)
                          ? memoryLayout.kernelSpaceEnd : memoryLayout.userSpaceEnd;

    if (address < start || address >= end)
        return false;

    if (count > (end - address) / memoryLayout.pageSize)
        return false;

    return true;
}

#define MAX_ADDRESSSPACE_REFERENCES 0xFFFFFFFF

Memory_Result Memory_AddressSpace_Create(AddressSpace** outAddressSpace)
{
    AddressSpace* newAddressSpace = Memory_KernelAllocate(sizeof(AddressSpace));
    if (!newAddressSpace)
        return MEMORY_ERROR_OUT_OF_MEMORY;

    Memory_Result result = Arch_CreateAddressSpace(&newAddressSpace->addressSpace);
    if (result != MEMORY_SUCCESS)
    {
        Memory_KernelFree(newAddressSpace);
        return result;
    }

    newAddressSpace->before = NULL;
    newAddressSpace->next = headAddressSpace;
    newAddressSpace->references = 1;

    if (headAddressSpace)
        headAddressSpace->before = newAddressSpace;
    headAddressSpace = newAddressSpace;

    *outAddressSpace = newAddressSpace;

    return MEMORY_SUCCESS;
}

Memory_Result Memory_AddressSpace_Get(AddressSpace* addressSpace)
{
    if (!addressSpace)
        return MEMORY_ERROR_NOT_REFERENCED;

    if (addressSpace->references >= MAX_ADDRESSSPACE_REFERENCES)
        return MEMORY_ERROR_REFERENCE_LIMIT;

    addressSpace->references++;

    return MEMORY_SUCCESS;
}

Memory_Result Memory_AddressSpace_Put(AddressSpace* addressSpace)
{
    if (!addressSpace)
        return MEMORY_ERROR_NOT_REFERENCED;
    
    if (addressSpace->references == 0)
        return MEMORY_ERROR_NOT_REFERENCED;

    if (--addressSpace->references == 0)
    {
        if (addressSpace->before)
            addressSpace->before->next = addressSpace->next;
        else
            headAddressSpace = addressSpace->next;

        if (addressSpace->next)
            addressSpace->next->before = addressSpace->before;

        Arch_DestroyAddressSpace(addressSpace->addressSpace);
        Memory_KernelFree(addressSpace);
    }

    return MEMORY_SUCCESS;
}

void Memory_AddressSpace_Use(AddressSpace* addressSpace)
{
    if (addressSpace == MEMORY_KERNEL)
        return;

    Arch_SwitchAddressSpace(addressSpace->addressSpace);
}


static inline Memory_Result link(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags, bool takePhysical)
{
    Memory_Result result;

    if (!takePhysical)
    {
        result = Memory_Physical_GetPage(physicalAddress);
        if (result != MEMORY_SUCCESS)
            return result;
    }

    result = (addressSpace == MEMORY_KERNEL)
             ? Memory_MapPageKernel(virtualAddress, physicalAddress, flags)
             : Arch_MapPage(addressSpace->addressSpace, virtualAddress, physicalAddress, flags);
    if (result != MEMORY_SUCCESS)
    {
        if (!takePhysical)
        {
            if (Memory_Physical_PutPage(physicalAddress) != MEMORY_SUCCESS)
                PanicMessageInfo("link", "Memory_Physical_PutPage(%q) failed\n", physicalAddress);
        }
        return result;
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_LinkNew(AddressSpace* addressSpace, uintptr_t virtualAddress, Memory_Flags flags, uintptr_t pageCount)
{
    if (!checkAddresses(addressSpace, virtualAddress, pageCount))
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    uphysptr_t physicalAddress;
    Memory_Result result;
    for (uintptr_t i = 0; i < pageCount; i++)
    {
        const uintptr_t addr = virtualAddress + i * memoryLayout.pageSize;

        result = Memory_Physical_NewPage(&physicalAddress);
        if (result != MEMORY_SUCCESS)
        {
            if (i > 0)
            {
                Memory_Result r = Memory_Unlink(addressSpace, virtualAddress, i);
                if (r != MEMORY_SUCCESS)
                    PanicMessageInfo("Memory_LinkNew", "Memory_Unlink(%p, %p, %p) failed\n", addressSpace, virtualAddress, i);
            }
            return result;
        }

        result = link(addressSpace, addr, physicalAddress, flags, true);
        if (result != MEMORY_SUCCESS)
        {
            if (Memory_Physical_PutPage(physicalAddress) != MEMORY_SUCCESS)
                PanicMessageInfo("Memory_LinkNew", "Memory_Physical_PutPage(%q) failed\n", physicalAddress);

            if (i > 0)
            {
                Memory_Result r = Memory_Unlink(addressSpace, virtualAddress, i);
                if (r != MEMORY_SUCCESS)
                    PanicMessageInfo("Memory_LinkNew", "Memory_Unlink(%p, %p, %p) failed\n", addressSpace, virtualAddress, i);
            }

            return result;
        }
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_Link(AddressSpace* addressSpace, uintptr_t virtualAddress, AddressSpace* sourceAddressSpace, uintptr_t sourceVirtualAddress, Memory_Flags flags, uintptr_t pageCount)
{
    if (!checkAddresses(addressSpace, virtualAddress, pageCount))
        return MEMORY_ERROR_DOMAIN;

    if (!checkAddresses(sourceAddressSpace, sourceVirtualAddress, pageCount))
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0 || sourceVirtualAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    uphysptr_t physicalAddress;
    Memory_Result result;

    for (uintptr_t i = 0; i < pageCount; i++)
    {
        const uintptr_t srcAddr = sourceVirtualAddress + i * memoryLayout.pageSize;

        result = Memory_Translate(sourceAddressSpace, srcAddr, &physicalAddress);
        if (result != MEMORY_SUCCESS)
            return result;
    }

    for (uintptr_t i = 0; i < pageCount; i++)
    {
        const uintptr_t addr = virtualAddress + i * memoryLayout.pageSize;
        const uintptr_t srcAddr = sourceVirtualAddress + i * memoryLayout.pageSize;

        result = Memory_Translate(sourceAddressSpace, srcAddr, &physicalAddress);
        if (result != MEMORY_SUCCESS)
        {
            PanicMessageInfo("Memory_Link", "Memory_Translate(%p, %p, %p) failed after working before\n", sourceAddressSpace, srcAddr, &physicalAddress);
            if (i > 0)
            {
                Memory_Result r = Memory_Unlink(addressSpace, virtualAddress, i);
                if (r != MEMORY_SUCCESS)
                    PanicMessageInfo("Memory_Link", "Memory_Unlink(%p, %p, %p) failed\n", addressSpace, virtualAddress, i);
            }
            return MEMORY_ERROR_INTERNAL;
        }

        result = link(addressSpace, addr, physicalAddress, flags, false);
        if (result != MEMORY_SUCCESS)
        {
            if (i > 0)
            {
                Memory_Result r = Memory_Unlink(addressSpace, virtualAddress, i);
                if (r != MEMORY_SUCCESS)
                    PanicMessageInfo("Memory_Link", "Memory_Unlink(%p, %p, %p) failed\n", addressSpace, virtualAddress, i);
            }
            return result;
        }
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_LinkRaw(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags)
{
    if (!checkAddresses(addressSpace, virtualAddress, 1))
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0 || physicalAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    return link(addressSpace, virtualAddress, physicalAddress, flags, false);
}

Memory_Result Memory_Unlink(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount)
{
    if (!checkAddresses(addressSpace, virtualAddress, pageCount))
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    Memory_Result result;
    uphysptr_t physicalAddress;
    for (uintptr_t i = 0; i < pageCount; i++)
    {
        const uintptr_t addr = virtualAddress + i * memoryLayout.pageSize;

        result = (addressSpace == MEMORY_KERNEL)
                 ? Memory_TranslateKernel(addr, &physicalAddress)
                 : Arch_TranslatePage(addressSpace->addressSpace, addr, &physicalAddress);
        if (result != MEMORY_SUCCESS)
            return result;
    }

    for (uintptr_t i = 0; i < pageCount; i++)
    {
        const uintptr_t addr = virtualAddress + i * memoryLayout.pageSize;

        result = (addressSpace == MEMORY_KERNEL)
                 ? Memory_TranslateKernel(addr, &physicalAddress)
                 : Arch_TranslatePage(addressSpace->addressSpace, addr, &physicalAddress);
        if (result != MEMORY_SUCCESS)
        {
            if (addressSpace == MEMORY_KERNEL)
                PanicMessageInfo("Memory_Unlink", "Memory_TranslateKernel(%p, %p) failed after working before\n", addr, &physicalAddress);
            else
                PanicMessageInfo("Memory_Unlink", "Arch_TranslatePage(%p, %p, %p) failed after working before\n", addressSpace->addressSpace, addr, &physicalAddress);
            continue;
        }

        result = (addressSpace == MEMORY_KERNEL)
                 ? Memory_UnmapPageKernel(addr)
                 : Arch_UnmapPage(addressSpace->addressSpace, addr);
        if (result != MEMORY_SUCCESS)
        {
            if (addressSpace == MEMORY_KERNEL)
                PanicMessageInfo("Memory_Unlink", "Memory_UnmapPageKernel(%p) failed\n", addr);
            else
                PanicMessageInfo("Memory_Unlink", "Arch_UnmapPage(%p, %p, %p) failed\n", addressSpace->addressSpace, addr);
            continue;
        }

        if ((result = Memory_Physical_PutPage(physicalAddress)) != MEMORY_SUCCESS)
            PanicMessageInfo("Memory_Unlink", "Memory_Physical_PutPage(%q) failed\n", physicalAddress);
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_Translate(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t* outPhysicalAddress)
{
    return (addressSpace == MEMORY_KERNEL)
           ? Memory_TranslateKernel(virtualAddress, outPhysicalAddress)
           : Arch_TranslatePage(addressSpace->addressSpace, virtualAddress, outPhysicalAddress);
}
