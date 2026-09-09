#pragma once

#include "physical.h"
#include "kernel.h"
#include "result.h"

#include <bootparams.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct AddressSpace AddressSpace;

#define MemoryLayout memoryLayout
extern Memory_Layout memoryLayout;

#define MEMORY_KERNEL NULL

Memory_Result Memory_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_MapPageKernel(uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Memory_UnmapPageKernel(uintptr_t virtualAddr);
Memory_Result Memory_TranslateKernel(uintptr_t virtualAddr, uphysptr_t* out);


Memory_Result Memory_AddressSpace_Create(AddressSpace** outAddressSpace);
Memory_Result Memory_AddressSpace_Get(AddressSpace* addressSpace);
Memory_Result Memory_AddressSpace_Put(AddressSpace* addressSpace);

Memory_Result Memory_LinkNew(AddressSpace* addressSpace, uintptr_t virtualAddress, Memory_Flags flags, uintptr_t pageCount);
Memory_Result Memory_Link(AddressSpace* addressSpace, uintptr_t virtualAddress, AddressSpace* sourceAddressSpace, uintptr_t sourceVirtualAddress, Memory_Flags flags, uintptr_t pageCount);
Memory_Result Memory_LinkRaw(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags);
Memory_Result Memory_Unlink(AddressSpace* addressSpace, uintptr_t virtualAddress, uintptr_t pageCount);
Memory_Result Memory_Translate(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t* outPhysicalAddress);

#define Memory_Kernel_AllocateVirtual(pageCount, out) Memory_KernelVirtual_AllocatePages(pageCount, out)
#define Memory_Kernel_FreeVirtual(address, pageCount) Memory_KernelVirtual_FreePages(address, pageCount)

#define Memory_GetPhysicalPage(out) Memory_Physical_NewPage(out)
#define Memory_PutPhysicalPage(page) Memory_Physical_PutPage(page)


void* Memory_KernelAllocate(uintptr_t size);
void* Memory_KernelZallocate(uintptr_t size);
void* Memory_KernelReallocate(void* ptr, uintptr_t newSize);
void* Memory_KernelRezallocate(void* ptr, uintptr_t newSize);
void Memory_KernelFree(void* ptr);
