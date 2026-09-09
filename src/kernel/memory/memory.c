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


Memory_Result Memory_Initialize(MemoryInfo* memoryInfo)
{
    Memory_Result result;

    Arch_Memory_Initialize(&memoryLayout);

    if ((result = Memory_Physical_Initialize(memoryInfo)) != MEMORY_SUCCESS)
        return result;

    if ((result = Memory_KernelVirtual_Initialize(memoryInfo)) != MEMORY_SUCCESS)
        return result;

    initialAddressSpace = memoryInfo->initialAddressSpace;

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
        Memory_Result result = Arch_SyncKernel(currentAS->addressSpace, current);

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
        Memory_Result result = Arch_SyncKernel(currentAS->addressSpace, current);

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

Memory_Result Memory_AddressSpace_Link(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags)
{
    if (virtualAddress < memoryLayout.userSpaceStart || virtualAddress >= memoryLayout.userSpaceEnd)
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0 || physicalAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    Memory_Result result;
    if ((result = Memory_Physical_GetPage(physicalAddress)) != MEMORY_SUCCESS)
        return result;

    if ((result = Arch_MapPage(addressSpace->addressSpace, virtualAddress, physicalAddress, flags)) != MEMORY_SUCCESS)
    {
        if (Memory_Physical_PutPage(physicalAddress) != MEMORY_SUCCESS)
            PanicMessageInfo("Memory_AddressSpace_Link", "Memory_Physical_PutPage(%q) failed\n", physicalAddress);
        return result;
    }

    return MEMORY_SUCCESS;
}

Memory_Result Memory_AddressSpace_Unlink(AddressSpace* addressSpace, uintptr_t virtualAddress)
{
    if (virtualAddress < memoryLayout.userSpaceStart || virtualAddress >= memoryLayout.userSpaceEnd)
        return MEMORY_ERROR_DOMAIN;

    if (virtualAddress % memoryLayout.pageSize != 0)
        return MEMORY_ERROR_NOT_ALIGNED;

    Memory_Result result;
    uphysptr_t physicalAddress;
    if ((result = Arch_TranslatePage(addressSpace->addressSpace, virtualAddress, &physicalAddress)) != MEMORY_SUCCESS)
        return result;

    if ((result = Arch_UnmapPage(addressSpace->addressSpace, virtualAddress)) != MEMORY_SUCCESS)
        return result;

    if ((result = Memory_Physical_PutPage(physicalAddress)) != MEMORY_SUCCESS)
        PanicMessageInfo("Memory_AddressSpace_Unlink", "Memory_Physical_PutPage(%q) failed\n", physicalAddress);

    return MEMORY_SUCCESS;
}

Memory_Result Memory_AddressSpace_Translate(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t* outPhysicalAddress)
{
    return Arch_TranslatePage(addressSpace->addressSpace, virtualAddress, outPhysicalAddress);
}
