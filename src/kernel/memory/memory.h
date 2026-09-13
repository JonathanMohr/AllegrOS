#pragma once

#include "kernel.h"
#include "result.h"

#include <bootparams.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Virtual_Memory_Area
{
    struct Virtual_Memory_Area* next;
    uintptr_t start;
    uintptr_t pageCount;
    
    Memory_Flags flags;
} Virtual_Memory_Area;

typedef struct AddressSpace
{
    uphysptr_t addressSpace;

    Virtual_Memory_Area* VMAs;
    
    struct AddressSpace* before;
    struct AddressSpace* next;

    uint32_t references;
} AddressSpace;

#define MemoryLayout memoryLayout
extern Memory_Layout memoryLayout;

#define MEMORY_PAGE_SIZE memoryLayout.pageSize

#define MEMORY_KERNEL NULL

Memory_Result Memory_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_MapPageKernel(uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Memory_UnmapPageKernel(uintptr_t virtualAddr);
Memory_Result Memory_TranslateKernel(uintptr_t virtualAddr, uphysptr_t* out);

AddressSpace* Memory_CurrentAddressSpace(void);

Memory_Result Memory_AddressSpace_Create(AddressSpace** outAddressSpace);
Memory_Result Memory_AddressSpace_Get(AddressSpace* addressSpace);
Memory_Result Memory_AddressSpace_Put(AddressSpace* addressSpace);
void Memory_AddressSpace_Use(AddressSpace* addressSpace);

Memory_Result Memory_LinkNew(AddressSpace* addressSpace, uintptr_t virtualAddress, Memory_Flags flags, uintptr_t pageCount);
Memory_Result Memory_Link(AddressSpace* addressSpace, uintptr_t virtualAddress, AddressSpace* sourceAddressSpace, uintptr_t sourceVirtualAddress, Memory_Flags flags, uintptr_t pageCount);
Memory_Result Memory_LinkRaw(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags);
Memory_Result Memory_Unlink(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount);
Memory_Result Memory_Translate(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t* outPhysicalAddress);
Memory_Result Memory_GetFlags(AddressSpace* addressSpace, uintptr_t virtualAddress, Memory_Flags* outFlags);
Memory_Result Memory_ChangeFlags(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount, Memory_Flags newFlags);

#define Memory_Kernel_AllocateVirtual(pageCount, out) Memory_KernelVirtual_AllocatePages(pageCount, out)
#define Memory_Kernel_FreeVirtual(address, pageCount) Memory_KernelVirtual_FreePages(address, pageCount)

Memory_Result Memory_AllocateVirtual(AddressSpace* addressSpace, uintptr_t pageCount, Memory_Flags flags, uintptr_t* out);
Memory_Result Memory_FreeVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount, bool strict);
Memory_Result Memory_ReserveVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount, Memory_Flags flags);
Memory_Result Memory_GetVirtual(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t* outStart, uintptr_t* outSize, Memory_Flags* outFlags);

void* Memory_KernelAllocate(uintptr_t size);
void* Memory_KernelZallocate(uintptr_t size);
void* Memory_KernelReallocate(void* ptr, uintptr_t newSize);
void* Memory_KernelRezallocate(void* ptr, uintptr_t newSize);
void Memory_KernelFree(void* ptr);
