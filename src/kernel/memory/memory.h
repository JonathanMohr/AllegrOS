#pragma once

#include "physical.h"
#include "result.h"

#include <bootparams.h>

typedef struct AddressSpace AddressSpace;

#define MemoryLayout memoryLayout
extern Memory_Layout memoryLayout;

Memory_Result Memory_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_MapPageKernel(uintptr_t virtualAddr, uphysptr_t physicalAddr, Memory_Flags flags);
Memory_Result Memory_UnmapPageKernel(uintptr_t virtualAddr);
Memory_Result Memory_TranslateKernel(uintptr_t virtualAddr, uphysptr_t* out);


Memory_Result Memory_AddressSpace_Create(AddressSpace** outAddressSpace);
Memory_Result Memory_AddressSpace_Get(AddressSpace* addressSpace);
Memory_Result Memory_AddressSpace_Put(AddressSpace* addressSpace);

Memory_Result Memory_AddressSpace_Link(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t physicalAddress, Memory_Flags flags);
Memory_Result Memory_AddressSpace_Unlink(AddressSpace* addressSpace, uintptr_t virtualAddress);
Memory_Result Memory_AddressSpace_Translate(AddressSpace* addressSpace, uintptr_t virtualAddress, uphysptr_t* outPhysicalAddress);


#define Memory_GetPhysicalPage(out) Memory_Physical_NewPage(out)
#define Memory_PutPhysicalPage(page) Memory_Physical_PutPage(page)


void* Memory_KernelAllocate(uintptr_t size);
void* Memory_KernelReallocate(void* ptr, uintptr_t newSize);
void Memory_KernelFree(void* ptr);
