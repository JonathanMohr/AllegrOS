#pragma once

#include <stdint.h>
#include "../drivers/disk/mbr.h"

void File_Init(Partition* part);
uint32_t File_Open(const char* path, uint32_t flags, uint32_t mode);
uint32_t File_Close(uint32_t handle);
uint32_t File_Read(uint32_t handle, uint8_t* buffer, uint32_t count);