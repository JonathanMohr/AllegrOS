#pragma once

#include <bootparams.h>

#include "result.h"

Memory_Result Memory_Physical_Initialize(MemoryInfo* memoryInfo);

Memory_Result Memory_Physical_NewPage(uphysptr_t* out);
Memory_Result Memory_Physical_GetPage(uphysptr_t page);
Memory_Result Memory_Physical_PutPage(uphysptr_t page);

Memory_Result Memory_Physical_Reserve(uphysptr_t pageStart, uphysptr_t count);
Memory_Result Memory_Physical_Unreserve(uphysptr_t pageStart, uphysptr_t count);
