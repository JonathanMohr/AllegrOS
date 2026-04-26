#pragma once

#include <stdint.h>
#include <abi.h>

void* CDECL memcpy(void* dst, const void* src, size_t n);
void* CDECL memset(void* dst, int value, size_t n);
void* CDECL memmove(void* dst, const void* src, size_t n);
