#pragma once

#include "result.h"

#include "../memory/memory.h"
#include "../filesystem/vfs.h"

ELF_Result ELF_Load(VFS_File* file, AddressSpace* addressSpace, bool currentAddressSpace, uintptr_t* outEntryPoint);
