#ifndef MEMORY_H
#define MEMORY_H

#include <abi.h>
#include <stdint.h>

int CDECL memcmp(const void* s1, const void* s2, size_t n);

void* CDECL memcpy(void* dst, const void* src, size_t n);
void* CDECL memset(void* dst, int value, size_t n);
void* CDECL memmove(void* dst, const void* src, size_t n);

#endif
