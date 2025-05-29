#pragma once

#include <stdint.h>

#define STDIN    0
#define STDOUT   1
#define STDERR   2
#define DEBUG    3

int32_t write(int fd, const void* buf, uint32_t count);
void fputc(char c, int file);
void fputs(const char* str, int file);