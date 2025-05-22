#pragma once

#include <boot/bootparams.h>

static MemoryRegion newRegions[MAX_REGIONS];

void Memory_AddBootRegion(MemoryInfo* memInfo);