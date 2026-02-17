#pragma once

#include <stdint.h>
#include "../drivers/disk/mbr.h"

void File_Init(Partition* part);
int32_t File_Open(const char* path, uint32_t flags, uint32_t mode);
int32_t File_Close(uint32_t fd);
int32_t File_Read(uint32_t fd, uint8_t* buffer, uint32_t count);
int32_t File_Write(uint32_t fd, uint8_t* buffer, uint32_t count);
int64_t File_Seek(uint32_t fd, int64_t offset, uint32_t whence);