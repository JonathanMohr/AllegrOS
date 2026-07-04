#pragma once

#include <stdbool.h>

#include "../device.h"

bool MBR_CheckDisk(Block_Device* device);

bool MBR_GetPartitionTable(Block_Device* device, Device_PartitionTable* table);
