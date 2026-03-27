#pragma once

#include <stdint.h>
#include <abi.h>

void* CDECL memcpy(void* dst, const void* src, uint32_t count);
void* CDECL memset(void* dst, int32_t value, uint32_t count);
