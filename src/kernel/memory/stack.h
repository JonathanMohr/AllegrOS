#pragma once

#include "memory.h"
#include "result.h"

Memory_Result AllocateUserStack(AddressSpace* addressSpace, uintptr_t* outStackTop);
