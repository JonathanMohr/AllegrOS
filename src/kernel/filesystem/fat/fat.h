#pragma once

#include "../filesystem.h"

bool FAT_CheckDevice(Block_Device* device);
bool FAT_GetDriver(Block_Device* parent, Filesystem_Driver* driver);
