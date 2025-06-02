#pragma once

#include <core/Defs.h>
#include <stdint.h>

void exit(int exit_code);
int32_t read(int fd, void* buf, uint32_t count);
int32_t write(int fd, const void* buf, uint32_t count);
int32_t open(const char *pathname, int flags, uint16_t mode);
int32_t close(int32_t fd);
int64_t lseek(int32_t fd, int64_t offset, int whence);